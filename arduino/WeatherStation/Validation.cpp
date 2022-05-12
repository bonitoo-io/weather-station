#include "Validation.h"
#include <AsyncJson.h>
#include "Settings.h"
#include <OpenWeatherMapCurrent.h>

static void writeResponseData(JsonObject &root,  ValidationStatus code, const char *message = nullptr);

ValidateParamsEndpoint::ValidateParamsEndpoint(const  char *uri):
  _uri(uri),
  _status(ValidationStatus::Idle) {
}

void ValidateParamsEndpoint::registerEndpoints(EndpointRegistrator *pRegistrator) {
    pRegistrator->registerGetHandler(_uri, std::bind(&ValidateParamsEndpoint::checkStatus, this, std::placeholders::_1, std::placeholders::_2));
    pRegistrator->registerPostHandler(_uri, std::bind(&ValidateParamsEndpoint::validateParams, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

static void writeResponseData(JsonObject &root,  ValidationStatus code, const char *message) {
  root[F("status")] = static_cast<int>(code);
  if(message) {
    root[F("message")] = message;
  }
}

void ValidateParamsEndpoint::validateParams(AsyncWebServerRequest* request, JsonVariant& json, route *) {
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

void  ValidateParamsEndpoint::checkStatus(AsyncWebServerRequest* request, route *) {
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


RegionalSettingsValidateEndpoint::RegionalSettingsValidateEndpoint(RegionalSettings *pCurrSettings, AdvancedSettings *pAdvSetting):
  ValidateParamsEndpoint(REGIONAL_SETTINGS_VALIDATE_ENDPOINT_PATH),
  _pCurrSettings(pCurrSettings),
  _pAdvSetting(pAdvSetting) {}


void RegionalSettingsValidateEndpoint::saveParams(JsonVariant& json) {
 if(_pNewSettings) {
    delete _pNewSettings;
  }
  JsonObject jsonObject = json.as<JsonObject>();
  _pNewSettings = new RegionalSettings;
  _pNewSettings->load(jsonObject);
}

void RegionalSettingsValidateEndpoint::runValidation() {
  if(_pCurrSettings->location != _pNewSettings->location) {
    OpenWeatherMapCurrentData data;
    OpenWeatherMapCurrent client;
    Serial.printf_P(PSTR(" Running regional validation: location: %s, lang %s, metrics: %d\n"),_pNewSettings->location.c_str(),_pNewSettings->language.c_str(), _pNewSettings->useMetricUnits);
    client.setLanguage(_pNewSettings->language);
    client.setMetric(_pNewSettings->useMetricUnits);
    client.updateCurrent(&data, _pAdvSetting->openWeatherAPIKey, _pNewSettings->location);
    if(!data.cityName.length()) {
      _error = F("City not found");
    }
  }
  delete _pNewSettings;
  _pNewSettings = nullptr;
}

AdvancedSettingsValidateEndpoint::AdvancedSettingsValidateEndpoint(RegionalSettings *pRegSettings):
  ValidateParamsEndpoint(ADVANCED_SETTINGS_VALIDATE_ENDPOINT_PATH),
  _pRegSettings(pRegSettings) {}

void AdvancedSettingsValidateEndpoint::saveParams(JsonVariant& json) {
 if(_pSettings) {
    delete _pSettings;
  }
  JsonObject jsonObject = json.as<JsonObject>();
  _pSettings = new AdvancedSettings;
  _pSettings->load(jsonObject);
}

void AdvancedSettingsValidateEndpoint::runValidation() {
  if(!strstr(_pSettings->openWeatherAPIKey.c_str(), ReplaceMark)) {
    OpenWeatherMapCurrentData data;
    OpenWeatherMapCurrent client;
    Serial.printf_P(PSTR(" Running advanced settings openeweathermap validation: location: %s, lang %s, metrics: %d\n"),_pRegSettings->location.c_str(),_pRegSettings->language.c_str(), _pRegSettings->useMetricUnits);
    client.setLanguage(_pRegSettings->language);
    client.setMetric(_pRegSettings->useMetricUnits);
    client.updateCurrent(&data, _pSettings->openWeatherAPIKey, _pRegSettings->location);
    if(!data.cityName.length()) {
      _error = F("Invalid API key");
    }
  }
  delete _pSettings;
  _pSettings = nullptr;
}