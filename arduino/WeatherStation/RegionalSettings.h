#ifndef WS_REGIONAL_SETTINGS_H
#define WS_REGIONAL_SETTINGS_H

#include <ESP8266WiFi.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include "Settings.h"

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


enum RegionalSettingsParts {
  DetectAutomatically = 1,
  City = 2,
  Language = 4,
  UtcOffset = 8,
  LocationCoords = 16,
  RegionParams = 32
};

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
    //calculated variables, do not store
    bool forceEngMessages;  
    uint8_t updatedParts;
public:
  RegionalSettings();
  virtual ~RegionalSettings() {}
  virtual int save(JsonObject& root) override;
  virtual int load(JsonObject& root) override;
  virtual void print(const __FlashStringHelper *title) override;
  virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/regionalSettings.json"); }
};

class RegionalSettingsEndpoint : public SettingsEndpoint {
  public:
    RegionalSettingsEndpoint(FSPersistence *pPersistence, RegionalSettings *pSettings);
};


#endif //WS_REGIONAL_SETTINGS_H
