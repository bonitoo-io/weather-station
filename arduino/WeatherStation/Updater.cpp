#include "Updater.h"
#include "Debug.h"
#include <ESPWiFi.h>

Updater::Updater() {

}

void Updater::init(const char *owner, const char *repo, const char *currentVersion, bool checkBeta) {
  _owner = owner;
  _repo = repo;
  _currentVersion = currentVersion;
  _checkBeta = checkBeta;
}


void Updater::checkUpdate() {
  WS_DEBUG_RAM("Before GH update");
  ESPGithubUpdater ghUpdater(_owner, _repo);
  ghUpdater.setRestartOnUpdate(false);
  String ver = ghUpdater.getLatestVersion(_checkBeta);
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
}
