
#include "Migrator.h"
#include "WiFiSettings.h"

bool Migrator::run() {
  _error = (char *)nullptr;
  return wifiMigration();
}

bool Migrator::wifiMigration() {
  String oldPath = F(FS_CONFIG_DIRECTORY "/" WIFI_SETTINGS_OLD_FILE_NAME);
  if(_pFsp->getFS()->exists(oldPath)) {
    // Wait to be sure serial console is ready to capture possible error
    delay(1000);
    WiFiSettings ws;
    ws.setFilePath(oldPath);
    _pFsp->readFromFS(&ws);
    String newName = F(WIFI_CONFIG_DIRECTORY "/");
    newName += ws.ssid;
    Serial.printf_P(PSTR("Migrating %s to %s\n"), oldPath.c_str(), newName.c_str());
    ws.setFilePath(newName);
    if(_pFsp->writeToFS(&ws)) {
      if(!_pFsp->removeConfig(oldPath)) {
        _error = F("Cannot remove ");
        _error += oldPath;
        return false;
      }
    } else {
      _error = F("Cannot write to ");
      _error += newName;
      return false;
    }
  }
  return true;
}
