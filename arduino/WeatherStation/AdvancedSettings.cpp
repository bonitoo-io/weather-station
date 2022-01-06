#include "AdvancedSettings.h"
#include "ScreenCommon.h"

const char *OpenweatherApiKeyStr PROGMEM = "openWeatherAPIKey";

AdvancedSettings::AdvancedSettings():
  updateDataInterval(ADVANCED_DEFAUT_UPDATE_INTERVAL),
  openWeatherAPIKey(ADVANCED_DEFAUT_OPENWEATHER_API_KEY),
  ntpServers(ADVANCED_DEFAUT_NTP_SERVERS),
  tempOffset(ADVANCED_DEFAUT_TEMPERATURE_OFFSET),
  humOffset(ADVANCED_DEFAUT_HUMIDITY_OFFSET),
  screenRotateInterval(ADVANCED_DEFAUT_SCREEN_ROTATE_INTERVAL),
  screens(ScreenConstants::getDefaultList()) {
    
}

void AdvancedSettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" updateDataInterval: "));Serial.print(updateDataInterval);
    Serial.print(F(", openWeatherAPIKey: "));Serial.print(obfuscateToken(openWeatherAPIKey));
    Serial.print(F(", ntpServers: "));Serial.print(ntpServers);
    Serial.print(F(", tempOffset: "));Serial.print(tempOffset);
    Serial.print(F(", humOffset: "));Serial.print(humOffset);
    Serial.print(F(", screenRotateInterval: "));Serial.print(screenRotateInterval);
    Serial.print(F(", screens: "));Serial.print(screens);
    Serial.println();
}

int AdvancedSettings::save(JsonObject& root) {
    root[F("updateDataInterval")] = updateDataInterval;
    root[FPSTR(OpenweatherApiKeyStr)] = openWeatherAPIKey;
    root[F("ntpServers")] = ntpServers;
    root[F("tempOffset")] = tempOffset;
    root[F("humOffset")] = humOffset;
    root[F("screenRotateInterval")] = screenRotateInterval;
    root[F("screens")] = screens;

    print(F("Save Advanced settings"));
    return 0;
}

int AdvancedSettings::load(JsonObject& root) {
  updateDataInterval = root[F("updateDataInterval")];
  openWeatherAPIKey = root[FPSTR(OpenweatherApiKeyStr)].as<const char *>();
  ntpServers = root[F("ntpServers")].as<const char *>();
  tempOffset = root[F("tempOffset")];
  humOffset = root[F("humOffset")];
  screenRotateInterval = root[F("screenRotateInterval")];
  screens = root[F("screens")].as<const char *>();

  print(F("Load Advanced settings"));
  return 0;
}


AdvancedSettingsEndpoint::AdvancedSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, AdvancedSettings *pSettings):
    SettingsEndpoint(pServer, ADVANCED_SETTINGS_ENDPOINT_PATH, pPersistence, pSettings, 
    [](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      AdvancedSettings *advSettings = (AdvancedSettings *)pSettings;
      if(advSettings->openWeatherAPIKey.length()>4) {
        jsonObject[FPSTR(OpenweatherApiKeyStr)] = obfuscateToken(advSettings->openWeatherAPIKey);
      }
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      const char *key = jsonObject[FPSTR(OpenweatherApiKeyStr)].as<const char *>();
      if(strstr(key, ReplaceMark)) {
        AdvancedSettings *advSettings = (AdvancedSettings *)pSettings;
        jsonObject[FPSTR(OpenweatherApiKeyStr)] = advSettings->openWeatherAPIKey;
      }
    }) {
    
}

