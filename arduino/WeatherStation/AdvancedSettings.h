#ifndef WS_ADVANCED_SETTINGS_H
#define WS_ADVANCED_SETTINGS_H

#include "Settings.h"
#include "RegionalSettings.h"


#define ADVANCED_DEFAUT_UPDATE_INTERVAL 60
#define ADVANCED_DEFAUT_OPENWEATHER_API_KEY ""
#define ADVANCED_DEFAUT_NTP_SERVERS "pool.ntp.org,time.nis.gov,time.google.com"
#define ADVANCED_DEFAUT_TEMPERATURE_OFFSET 0
#define ADVANCED_DEFAUT_HUMIDITY_OFFSET 0
#define ADVANCED_DEFAULT_OWNER F("bonitoo-io")
#define ADVANCED_DEFAULT_REPO F("weather-station")
#define ADVANCED_DEFAULT_BIN_FILE F("ws-firmware-%version%.bin")
#define ADVANCED_DEFAULT_MD5_FILE F("ws-firmware-%version%.md5")
#define ADVANCED_DEFAULT_UPDATETIME 300 //HHMM
#define ADVANCED_DEFAULT_CHECKBETA  false
#define ADVANCED_DEFAULT_VERIFY_CERT  true
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
    // Update repo owner
    String owner;
    // Update repo name
    String repo;
    // Update binary filename
    String binFile;
    // Update md5 filename
    String md5File;
    // Update time H*100+min
    uint16_t updateTime;
    // Update to beta releases
    bool checkBeta;
    // Verify github cert
    bool verifyCert;
  protected:
    void setUpdateTime(uint16_t time);
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
    AdvancedSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, AdvancedSettings *pSettings, RegionalSettings *pRegionalSettings);
  protected:
    RegionalSettings *_pRegionalSettings;
};

#endif //WS_ADVANCED_SETTINGS_H