
#include "Migrator.h"
#include "WiFiSettings.h"

bool Migrator::run() {
  _error = (char *)nullptr;
  return wifiMigration();
}

bool Migrator::wifiMigration() {
  String oldPath = F(FS_CONFIG_DIRECTORY "/" WIFI_SETTINGS_OLD_FILE_NAME);
  if(_pFsp->getFS()->exists(oldPath)) {
    WiFiSettings ws;
    ws.setFilePath(oldPath);
    _pFsp->readFromFS(&ws);
    String newName = F(WIFI_CONFIG_DIRECTORY "/");
    newName += ws.ssid;
    Serial.printf_P(PSTR("Migrating %s to %s\n"), oldPath.c_str(), newName.c_str());
    if(!_pFsp->getFS()->rename(oldPath, newName)) {
      _error = F("Cannot rename ");
      _error += oldPath;
      _error += F(" to ");
      _error += newName;
      return false;
    }
  }
  return true;
}
