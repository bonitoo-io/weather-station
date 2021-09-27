#ifndef UPDATER_H
#define UPDATER_H

#include "Settings.h"
#include <ESPGithubUpdater.h>
#include "RegionalSettings.h"

typedef std::function<void(const char *newVersion, int progress)> FWUpdateProgressCallback;
typedef std::function<void(const char *newVersion)> FWUpdateStartedCallback;
typedef std::function<void(bool success, const char *error)> FWUpdateFinishedCallback;


#define UPDATER_DEFAULT_OWNER F("bonitoo-io")
#define UPDATER_DEFAULT_REPO F("weather-station")
#define UPDATER_DEFAULT_BIN_FILE F("ws-firmware-%version%.bin")
#define UPDATER_DEFAULT_MD5_FILE F("ws-firmware-%version%.md5")
#define UPDATER_DEFAULT_UPDATETIME 300 //HHMM
#define UPDATER_DEFAULT_CHECKBETA  true
#define UPDATER_DEFAULT_VERIFY_CERT  true
#define UPDATER_SETTINGS_ENDPOINT_PATH "/api/updaterSettings"

class UpdaterSettings : public Settings {
 public:
  String owner;
  String repo;
  String binFile;
  String md5File;
  uint16_t updateTime;
  bool checkBeta;
  bool verifyCert;
 public:
  UpdaterSettings();
  virtual int save(JsonObject& root) override;
  virtual int load(JsonObject& root) override;
  virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/updaterSettings.json"); }  
};

class Updater {
 public:
  Updater();
  void init(UpdaterSettings *settings, const char *currentVersion);
  bool checkUpdate();
  void setUpdateCallbacks(FWUpdateStartedCallback startCb, FWUpdateProgressCallback progCb, FWUpdateFinishedCallback endCb) {
    _startCb = startCb;
    _progCb = progCb;
    _endCb = endCb;
  }
 private:
  UpdaterSettings *_settings;
  const char *_currentVersion = nullptr;
  FWUpdateStartedCallback _startCb = nullptr;
  FWUpdateProgressCallback _progCb = nullptr;
  FWUpdateFinishedCallback _endCb = nullptr;
};

class UpdaterSettingEnpoint : public SettingsEndpoint {
 public:
  UpdaterSettingEnpoint(AsyncWebServer* pServer, FSPersistence *pPersistence, 
        UpdaterSettings *pSettings, RegionalSettings *pRegionalSettings);
 protected:
  RegionalSettings *_pRegionalSettings;
};

#endif //UPDATER_H
