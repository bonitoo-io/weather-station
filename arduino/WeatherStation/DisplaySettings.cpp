#include "DisplaySettings.h"
#include "ScreenCommon.h"

DisplaySettings::DisplaySettings():
  screenRotateInterval(DISPLAY_DEFAUT_SCREEN_ROTATE_INTERVAL),
  screens(ScreenConstants::getDefaultList()) {
    
}

void DisplaySettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" screenRotateInterval: "));Serial.print(screenRotateInterval);
    Serial.print(F(", screens: "));Serial.print(screens);
    Serial.println();
}

int DisplaySettings::save(JsonObject& root) {
    root[F("screenRotateInterval")] = screenRotateInterval;
    root[F("screens")] = screens;

    print(F("Save Display settings"));
    return 0;
}

int DisplaySettings::load(JsonObject& root) {
  screenRotateInterval = root[F("screenRotateInterval")];
  screens = root[F("screens")].as<const char *>();

  print(F("Load Display settings"));
  return 0;
}


DisplaySettingsEndpoint::DisplaySettingsEndpoint(AsyncWebServer* pServer, FSPersistence *pPersistence, DisplaySettings *pSettings):
    SettingsEndpoint(pServer, F(DISPLAY_SETTINGS_ENDPOINT_PATH), pPersistence, pSettings)
     {
    
}

