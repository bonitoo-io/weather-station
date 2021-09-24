#include "Validation.h"
#include <AsyncJson.h>
#include "Settings.h"

static void writeResponseData(JsonObject &root,  ValidationStatus code, const char *message = nullptr);

ValidateParamsEndpoint::ValidateParamsEndpoint(AsyncWebServer* server, const char *uri):
  _status(ValidationStatus::Idle) {
  AsyncCallbackJsonWebHandler *updateHandler = new AsyncCallbackJsonWebHandler(uri, 
                    std::bind(&ValidateParamsEndpoint::validateParams, this, std::placeholders::_1, std::placeholders::_2),
                    DEFAULT_BUFFER_SIZE);
  updateHandler->setMethod(HTTP_POST);
  server->addHandler(updateHandler);
  server->on(uri, HTTP_GET, std::bind(&ValidateParamsEndpoint::checkStatus, this, std::placeholders::_1));
}

static void writeResponseData(JsonObject &root,  ValidationStatus code, const char *message) {
  root[F("status")] = static_cast<int>(code);
  if(message) {
    root[F("message")] = message;
  }
}

void ValidateParamsEndpoint::validateParams(AsyncWebServerRequest* request, JsonVariant& json) {
    if (!json.is<JsonObject>()) {
        request->send(400);
        return;
    }
    ValidationStatus responseState = _status;
    if(_status == ValidationStatus::Idle || _status >= ValidationStatus::Finished) {
      saveParams(json);
      request->onDisconnect([this]() {
        _status = ValidationStatus::StartRequest;
      });
      responseState = ValidationStatus::StartRequest;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    JsonObject responseJson = response->getRoot().to<JsonObject>();    
    writeResponseData(responseJson, responseState);
    response->setLength();
    response->setCode(200); 
    request->send(response);
}



void ValidateParamsEndpoint::loop() {
  if(_status == ValidationStatus::StartRequest) {
    _status = ValidationStatus::Running;
    runValidation();
    Serial.print(F(" Validation result: "));
    if(!_error.length()) {
       Serial.println(F(" success"));
      _status = ValidationStatus::Finished;
    } else {
      Serial.print(F(" error "));
      Serial.println(_error);
      _status = ValidationStatus::Error;
    }
  }
}

void  ValidateParamsEndpoint::checkStatus(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject jsonObject = response->getRoot().to<JsonObject>();
  if (_status < ValidationStatus::Finished) {
    writeResponseData(jsonObject, _status);
  } else {
    if(_status == ValidationStatus::Error && _error.length() && _error[0] == '{') {
      DynamicJsonDocument doc(DEFAULT_BUFFER_SIZE);
      deserializeJson(doc, _error.c_str());
      _error = doc[F("message")].as<const char *>();
    }
    writeResponseData(jsonObject,_status, _status == ValidationStatus::Error?_error.c_str():nullptr);
    request->onDisconnect([this]() {
      _status = ValidationStatus::Idle;
      _error = (char *)nullptr;
    });
  }
  response->setLength();
  response->setCode(200);
  request->send(response);
}
