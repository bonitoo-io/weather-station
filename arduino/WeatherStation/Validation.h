
#ifndef WS_VALIDATION_HELPER_H
#define WS_VALIDATION_HELPER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

enum class ValidationStatus {
  Idle = 0,
  StartRequest,
  Running,
  Finished,
  Error
};

class ValidateParamsEndpoint {
public:
  ValidateParamsEndpoint(AsyncWebServer* server, const char *uri);
  virtual ~ValidateParamsEndpoint() {};
  void loop();
protected:
  // Should read json data and save params for later validation
  virtual void saveParams(JsonVariant& json) = 0;
  // Does the validation. Should set _error if sth wrong happens
  virtual void runValidation() = 0;
private:
  // HTTP handlers
  void validateParams(AsyncWebServerRequest* request, JsonVariant& json);
  void checkStatus(AsyncWebServerRequest* request);
protected:
    ValidationStatus _status;
    String _error;
};


#endif // WS_VALIDATION_HELPER_H
