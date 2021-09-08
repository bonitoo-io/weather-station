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
  WS_DEBUG_RAM("Before GH update");
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
    Serial.printf_P(PSTR("Updater: Curent version %s, latest version: %s\n"), _currentVersion, ver.c_str());
    if(ver != _currentVersion) {
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
  WS_DEBUG_RAM("After GH update");
  return res;
}

void printUpdaterSettings(String prefix, UpdaterSettings *s) {
    Serial.print(prefix);
    Serial.print(F(" owner: "));Serial.print(s->owner);
    Serial.print(F(", repo: "));Serial.print(s->repo);
    Serial.print(F(", binFile: "));Serial.print(s->binFile);
    Serial.print(F(", md5File: "));Serial.print(s->md5File);
    Serial.print(F(", updateTime: "));Serial.print(s->updateTime);
    Serial.print(F(", checkBeta: "));Serial.print(s->checkBeta);
    Serial.print(F(", verifyCert: "));Serial.print(s->verifyCert);
    Serial.println();
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

int UpdaterSettings::save(JsonObject& root) {
    root[F("owner")] = owner;
    root[F("repo")] = repo;
    root[F("binFile")] = binFile;
    root[F("md5File")] = md5File;
    root[F("updateTime")] = updateTime;
    root[F("checkBeta")] = checkBeta;
    root[F("verifyCert")] = verifyCert;
    printUpdaterSettings(F("UpdaterSettings::Save"), this);
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
    printUpdaterSettings(F("UpdaterSettings::Load"), this);
    return 1;
}