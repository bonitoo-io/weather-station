#include "Updater.h"
#include "Debug.h"
#include <ESPWiFi.h>


Updater::Updater() {

}

void Updater::init(AdvancedSettings *settings, const char *currentVersion) {
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
