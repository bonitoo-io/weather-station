#ifndef WS_REGIONAL_SETTINGS_H
#define WS_REGIONAL_SETTINGS_H

#include <ESP8266WiFi.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include "Settings.h"
#include "Validation.h"
#include "Tools.h"

#define REGIONAL_SETTINGS_DEFAULT_DETECT true
#define REGIONAL_SETTINGS_DEFAULT_CITY F("San Francisco,US")
#define REGIONAL_SETTINGS_DEFAULT_LANGUAGE F("en")
#define REGIONAL_SETTINGS_DEFAULT_UTC_OFFSET -25200
#define REGIONAL_SETTINGS_DEFAULT_LATITUDE 37.7749 
#define REGIONAL_SETTINGS_DEFAULT_LONGITUDE  -122.4194
#define REGIONAL_SETTINGS_DEFAULT_USE_METRICS  false
#define REGIONAL_SETTINGS_DEFAULT_USE_24HOURS  false
#define REGIONAL_SETTINGS_DEFAULT_USE_YMD  true
#define REGIONAL_SETTINGS_ENDPOINT_PATH "/api/regionalSettings"
#define REGIONAL_SETTINGS_VALIDATE_ENDPOINT_PATH "/api/regionalSettingsCheckValidation"

class RegionalSettings : public Settings {
public:
    bool detectAutomatically;
    // OpenWeatherMap City 
    // Go to https://openweathermap.org/find?q= and search for a location  
    String location;
    // language code. Currently supported en, cz 
    String language;
    // in seconds
    int utcOffset;
    float latitude;
    float longitude;
    bool useMetricUnits;
    bool use24Hours;
    bool useYMDFormat;
public:
  RegionalSettings();
  virtual int save(JsonObject& root) override;
  virtual int load(JsonObject& root) override;
  virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/regionalSettings.json"); }  
};

class RegionalSettingsValidateEndpoint : public ValidateParamsEndpoint {
public:
    RegionalSettingsValidateEndpoint(AsyncWebServer* server, tConfig *pConf);
    virtual ~RegionalSettingsValidateEndpoint() { delete _pSettings; }
protected:
  virtual void saveParams(JsonVariant& json) override;
  virtual void runValidation() override;
private:
  tConfig *_pConf;
  RegionalSettings *_pSettings = nullptr;
};

#endif //WS_REGIONAL_SETTINGS_H
