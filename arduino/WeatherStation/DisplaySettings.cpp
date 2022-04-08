#include "DisplaySettings.h"
#include "ScreenCommon.h"

DisplaySettings::DisplaySettings():
  screenRotateInterval(DISPLAY_DEFAUT_SCREEN_ROTATE_INTERVAL),
  screens(ScreenConstants::getDefaultList()),
  nightModeBegin(DISPLAY_DEFAUT_NIGHT_MODE_BEGIN),
  nightModeEnd(DISPLAY_DEFAUT_NIGHT_MODE_END) {
    
}

void DisplaySettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" screenRotateInterval: "));Serial.print(screenRotateInterval);
    Serial.print(F(", screens: "));Serial.print(screens);
    Serial.print(F(", nightModeBegin: "));Serial.print(nightModeBegin);
    Serial.print(F(", nightModeEnd: "));Serial.print(nightModeEnd);
    Serial.println();
}

int DisplaySettings::save(JsonObject& root) {
    root[F("screenRotateInterval")] = screenRotateInterval;
    root[F("screens")] = screens;
    root[F("nightModeBegin")] = nightModeBegin;
    root[F("nightModeEnd")] = nightModeEnd;

    print(F("Save Display settings"));
    return 0;
}

int DisplaySettings::load(JsonObject& root) {
  screenRotateInterval = root[F("screenRotateInterval")];
  screens = root[F("screens")].as<const char *>();
  nightModeBegin = root[F("nightModeBegin")] | DISPLAY_DEFAUT_NIGHT_MODE_BEGIN ;
  nightModeEnd = root[F("nightModeEnd")] | DISPLAY_DEFAUT_NIGHT_MODE_END;
  print(F("Load Display settings"));
  return 0;
}


DisplaySettingsEndpoint::DisplaySettingsEndpoint(AsyncWebServer* pServer, FSPersistence *pPersistence, DisplaySettings *pSettings, RegionalSettings *pRegionalSettings):
    SettingsEndpoint(pServer, F(DISPLAY_SETTINGS_ENDPOINT_PATH), pPersistence, pSettings, [this](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      jsonObject[F("use24Hours")] = _pRegionalSettings->use24Hours;
    }),  _pRegionalSettings(pRegionalSettings) {
    
}
