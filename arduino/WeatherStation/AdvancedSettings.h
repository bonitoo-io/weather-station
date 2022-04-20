#ifndef WS_ADVANCED_SETTINGS_H
#define WS_ADVANCED_SETTINGS_H

#include "Settings.h"
#include "RegionalSettings.h"
#include "EEPROMData.h"

#define ADVANCED_SETTINGS_ENDPOINT_PATH "/api/advancedSettings"
#define ADVANCED_SETTINGS_VALIDATE_ENDPOINT_PATH "/api/validateAdvancedSettings"

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
  private:
    EEPROMData _eepromData;
  public:
    AdvancedSettings();
    virtual ~AdvancedSettings() {};

    void begin();
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual void print(const __FlashStringHelper *title) override;
    virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/advancedSettings.json"); }
    void updateEEPROMData();
};

class AdvancedSettingsEndpoint : public SettingsEndpoint {
  public:
    AdvancedSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, AdvancedSettings *pSettings, RegionalSettings *pRegionalSettings);
  protected:
    RegionalSettings *_pRegionalSettings;
};

#endif //WS_ADVANCED_SETTINGS_H