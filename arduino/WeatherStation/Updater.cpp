#include "Updater.h"
#include "Debug.h"
#include <ESPWiFi.h>


Updater::Updater() {

}

void Updater::init(UpdaterSettings *settings, const char *currentVersion) {
  _settings = settings;
  _currentVersion = currentVersion;
}


bool Updater::checkUpdate() {
  ESPGithubUpdater ghUpdater(_settings->owner, _settings->repo, _settings->binFile);
  ghUpdater.setMD5FileName(_settings->md5File);
  ghUpdater.setRestartOnUpdate(false);
  if(!_settings->verifyCert) {
    ghUpdater.setInsecure();
  }

  Serial.printf_P(PSTR("Updater: checking  update for %s/%s, check for beta: %d\n"),_settings->owner.c_str(), _settings->repo.c_str(), _settings->checkBeta); 
  String ver = ghUpdater.getLatestVersion(_settings->checkBeta);
  bool res = ver.length();
  if(res) {
    int c = strcmp(_currentVersion, ver.c_str());
    Serial.printf_P(PSTR("Updater: Curent version '%s', latest version: '%s'. Cmp result: %d\n"), _currentVersion, ver.c_str(), c);
    // check if new version is higher or final 
    // e.g. latest: 0.58, current 0.58-rc-8
    if( c < 0 || (c > 0 && strstr(_currentVersion, ver.c_str()) )) {
      Serial.println(F("Updater: Starting update"));
      if(_startCb) {
        _startCb(ver.c_str());
      }
      res = ghUpdater.runUpdate(ver, [this, ver](int progress) {
        if(_progCb) {
          _progCb(ver.c_str(), progress);
        }
      });
      String err;
      if(res) {
        Serial.println(F("Update successful"));
      } else {
        err = ghUpdater.getLastError();
      } 
      if(_endCb) {
        _endCb(res, err.c_str());
      }
    }
  } 
  if(!res) {
    Serial.print(F("Updater: Error "));
    Serial.println(ghUpdater.getLastError());
  }
  return res;
}

UpdaterSettings::UpdaterSettings():
  owner(UPDATER_DEFAULT_OWNER),
  repo(UPDATER_DEFAULT_REPO),
  binFile(UPDATER_DEFAULT_BIN_FILE),
  md5File(UPDATER_DEFAULT_MD5_FILE),
  updateTime(UPDATER_DEFAULT_UPDATETIME),
  checkBeta(UPDATER_DEFAULT_CHECKBETA),
  verifyCert(UPDATER_DEFAULT_VERIFY_CERT) {
}

void UpdaterSettings::print(const __FlashStringHelper *title) {
  Serial.print(title);
  Serial.print(F(" owner: "));Serial.print(owner);
  Serial.print(F(", repo: "));Serial.print(repo);
  Serial.print(F(", binFile: "));Serial.print(binFile);
  Serial.print(F(", md5File: "));Serial.print(md5File);
  Serial.print(F(", updateTime: "));Serial.print(updateTime);
  Serial.print(F(", checkBeta: "));Serial.print(checkBeta);
  Serial.print(F(", verifyCert: "));Serial.print(verifyCert);
  Serial.println();
}

int UpdaterSettings::save(JsonObject& root) {
  root[F("owner")] = owner;
  root[F("repo")] = repo;
  root[F("binFile")] = binFile;
  root[F("md5File")] = md5File;
  root[F("updateTime")] = updateTime;
  root[F("checkBeta")] = checkBeta;
  root[F("verifyCert")] = verifyCert;
  print(F("UpdaterSettings::Save"));
  return 0;
}

int UpdaterSettings::load(JsonObject& root) {
    owner = root[F("owner")].as<const char *>();
    repo = root[F("repo")].as<const char *>();
    binFile = root[F("binFile")].as<const char *>();
    md5File = root[F("md5File")].as<const char *>();;
    updateTime = root[F("updateTime")];
    checkBeta = root[F("checkBeta")];
    verifyCert = root[F("verifyCert")] | UPDATER_DEFAULT_VERIFY_CERT;
    print(F("UpdaterSettings::Load"));
    return 1;
}

UpdaterSettingEnpoint::UpdaterSettingEnpoint(AsyncWebServer* pServer, FSPersistence *pPersistence, 
        UpdaterSettings *pSettings, RegionalSettings *pRegionalSettings)
  :SettingsEndpoint(pServer, UPDATER_SETTINGS_ENDPOINT_PATH, pPersistence, pSettings, 
  [this](Settings *, JsonObject jsonObject) {
    jsonObject[F("use24Hours")] = _pRegionalSettings->use24Hours;
  }),
  _pRegionalSettings(pRegionalSettings) { }

