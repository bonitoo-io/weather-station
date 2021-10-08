#ifndef WS_ADVANCED_SETTINGS_H
#define WS_ADVANCED_SETTINGS_H

#include "Settings.h"


#define ADVANCED_DEFAUT_UPDATE_INTERVAL 60
#define ADVANCED_DEFAUT_OPENWEATHER_API_KEY ""
#define ADVANCED_DEFAUT_NTP_SERVERS "pool.ntp.org,time.nis.gov,time.google.com"
#define ADVANCED_DEFAUT_TEMPERATURE_OFFSET 0
#define ADVANCED_DEFAUT_HUMIDITY_OFFSET 0
#define ADVANCED_DEFAUT_SCREEN_ROTATE_INTERVAL 10
#define ADVANCED_SETTINGS_ENDPOINT_PATH "/api/advancedSettings"
#define ADVANCED_SETTINGS_VALIDATE_ENDPOINT_PATH "/api/validateAdvancedSettings"

#include "custom_dev.h"

class AdvancedSettings : public Settings {
  public:
    // Update data (etc weather forecast) intreval in minutes
    uint16_t updateDataInterval;
    // OpenWeather API key, https://openweathermap.org/price
    String openWeatherAPIKey;
    // Comma separated list of 1-3 NTP servers
    String ntpServers;
    // Temperature compenstation coefficient
    float tempOffset;
    // humidity compenstation coefficient
    float humOffset;
    // Interval of screens changes, in seconds
    uint8_t screenRotateInterval;
    // Letters of visible screens
    String screens;
  public:
    AdvancedSettings();
    virtual ~AdvancedSettings() {};
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual void print(const __FlashStringHelper *title) override;
    virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/advancedSettings.json"); }
};

class AdvancedSettingsEndpoint : public SettingsEndpoint {
public:
    AdvancedSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, AdvancedSettings *pSettings);
};

#endif //WS_ADVANCED_SETTINGS_H