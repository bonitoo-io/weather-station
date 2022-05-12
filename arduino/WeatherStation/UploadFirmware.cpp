#include "UploadFirmware.h"
#include "About.h"

UploadFirmwareEndpoint::UploadFirmwareEndpoint(AsyncWebServer* server) {
  server->on(F(UPLOAD_FIRMWARE_PATH),
             HTTP_POST,
             std::bind(&UploadFirmwareEndpoint::uploadComplete, this, std::placeholders::_1),
             std::bind(&UploadFirmwareEndpoint::handleUpload,
                       this,
                       std::placeholders::_1,
                       std::placeholders::_2,
                       std::placeholders::_3,
                       std::placeholders::_4,
                       std::placeholders::_5,
                       std::placeholders::_6));
  Update.runAsync(true);
}

void UploadFirmwareEndpoint::handleUpload(AsyncWebServerRequest* request,
                                         const String& /*filename*/,
                                         size_t index,
                                         uint8_t* data,
                                         size_t len,
                                         bool final) {
  if (!index) {
    if (Update.begin(request->contentLength())) {
      // success, let's make sure we end the update if the client hangs up
      Serial.println(F("Starting upload FW"));
      request->onDisconnect([this]() {
        handleEarlyDisconnect();
      });
    } else {
      // failed to begin, send an error response
      Update.printError(Serial);
      handleError(request, 500);
    }
  }

  // if we haven't dealt with an error, continue with the update
  if (!request->_tempObject) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
      handleError(request, 500);
    }
    if (final) {
      if (!Update.end(true)) {
        Update.printError(Serial);
        handleError(request, 500);
      }
    }
  }
}

void UploadFirmwareEndpoint::loop() {
  if(_notify) {
    if(_notify == 1 && _startedCallback) {
      _startedCallback();
    } else if(_notify > 1) {
      if(_finishedCallback) {
        _finishedCallback(_notify == 2);
      } else {
        AboutServiceEndpoint::restartNow();
      }
    }
    _notify = 0;
  }
}

void UploadFirmwareEndpoint::uploadComplete(AsyncWebServerRequest* request) {
  // if no error, send the success response
  if (!request->_tempObject) {
    request->onDisconnect([this]() {
      // Must notify in main loop, not async callbalk
      Serial.println(F("Upload FW successfully completed"));
      _notify = 2;
    });
    AsyncWebServerResponse* response = request->beginResponse(200);
    request->send(response);
  }
}

void UploadFirmwareEndpoint::handleError(AsyncWebServerRequest* request, int code) {
  // if we have had an error already, do nothing
  Serial.println(F("Upload FW failed"));
  if (request->_tempObject) {
    return;
  }
  // send the error code to the client and record the error code in the temp object
  request->_tempObject = new int(code);
  AsyncWebServerResponse* response = request->beginResponse(code);
  request->onDisconnect([this]() {
    _notify = 3;
  });
  request->send(response);
}

void UploadFirmwareEndpoint::handleEarlyDisconnect() {
  Serial.println(F("Upload FW failed"));
  _notify = 3;
  Update.end();
}
