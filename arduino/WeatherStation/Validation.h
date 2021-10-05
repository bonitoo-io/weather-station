
#ifndef WS_VALIDATION_HELPER_H
#define WS_VALIDATION_HELPER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "RegionalSettings.h"
#include "AdvancedSettings.h"

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


class RegionalSettingsValidateEndpoint : public ValidateParamsEndpoint {
public:
    RegionalSettingsValidateEndpoint(AsyncWebServer* server, AdvancedSettings *pAdvSetting);
    virtual ~RegionalSettingsValidateEndpoint() { delete _pSettings; }
protected:
  virtual void saveParams(JsonVariant& json) override;
  virtual void runValidation() override;
private:
  AdvancedSettings *_pAdvSetting;
  RegionalSettings *_pSettings = nullptr;
};

class AdvancedSettingsValidateEndpoint : public ValidateParamsEndpoint {
public:
    AdvancedSettingsValidateEndpoint(AsyncWebServer* server, RegionalSettings *pRegSetting);
    virtual ~AdvancedSettingsValidateEndpoint() { delete _pSettings; }
protected:
  virtual void saveParams(JsonVariant& json) override;
  virtual void runValidation() override;
private:
  AdvancedSettings *_pSettings = nullptr;
  RegionalSettings *_pRegSettings;
};

#endif // WS_VALIDATION_HELPER_H
