#ifndef WS_ADVANCED_SETTINGS_H
#define WS_ADVANCED_SETTINGS_H

#include "Settings.h"
#include "RegionalSettings.h"
#include "EEPROMData.h"

#define ADVANCED_SETTINGS_ENDPOINT_PATH "/api/advancedSettings"
#define ADVANCED_SETTINGS_VALIDATE_ENDPOINT_PATH "/api/validateAdvancedSettings"

enum AdvancedSettingsParts {
  UpdateInterval = 1,
  OpenWeatherAPIKey = 2,
  NTPServers = 4,
  UpdateSettings = 8
};

class AdvancedSettings : public Settings {
  public:
    // Update data (etc weather forecast) interval in minutes
    uint16_t updateDataInterval;
    // OpenWeather API key, https://openweathermap.org/price
    String openWeatherAPIKey;
    // Comma separated list of 1-3 NTP servers
    String ntpServers;
    //tempOffset - handled via get/setTempOffset
    //humOffset - handled via get/setHumOffset
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
    // not saved field
    uint8_t updatedParts;
  protected:
    void setUpdateTime(uint16_t time);
  private:
    EEPROMData _eepromData; //temperature and humidity offsets
  public:
    AdvancedSettings();
    virtual ~AdvancedSettings() {};

    void begin();
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual void print(const __FlashStringHelper *title) override;
    virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/advancedSettings.json"); }
    void updateEEPROMData( float tempOffset, float humOffset);
    float getTempOffset();
    float getTempOffsetF();
    float getHumOffset();
    void setTempOffset( float tempOffset);  //does not store offset persistently - use _eepromData.write
    void setHumOffset( float humOffset);  //does not store offset persistently - use _eepromData.write
};

class AdvancedSettingsEndpoint : public SettingsEndpoint {
  public:
    AdvancedSettingsEndpoint(FSPersistence *pPersistence, AdvancedSettings *pSettings, RegionalSettings *pRegionalSettings);
  protected:
    RegionalSettings *_pRegionalSettings;
};

#endif //WS_ADVANCED_SETTINGS_H
