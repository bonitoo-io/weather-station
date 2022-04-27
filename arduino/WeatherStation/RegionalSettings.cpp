#include "RegionalSettings.h"

bool nonLatin2Eng( const char* lang); //from location.cpp

void RegionalSettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" detectAutomatically: "));Serial.print(detectAutomatically);
    Serial.print(F(", location: "));Serial.print(location);
    Serial.print(F(", language: "));Serial.print(language);
    Serial.print(F(", utcOffset: "));Serial.print(utcOffset);
    Serial.print(F(", latitude: "));Serial.print(latitude);
    Serial.print(F(", longitude: "));Serial.print(longitude);
    Serial.print(F(", useMetricUnits: "));Serial.print(useMetricUnits);
    Serial.print(F(", use24Hours: "));Serial.print(use24Hours);
    Serial.print(F(", useYMDFormat: "));Serial.print(useYMDFormat);
    Serial.print(F(", forceEngMessages: "));Serial.print(forceEngMessages);
    Serial.println();
}


RegionalSettings::RegionalSettings():
  detectAutomatically(REGIONAL_SETTINGS_DEFAULT_DETECT),
  location(REGIONAL_SETTINGS_DEFAULT_CITY),
  language(REGIONAL_SETTINGS_DEFAULT_LANGUAGE),
  utcOffset(REGIONAL_SETTINGS_DEFAULT_UTC_OFFSET),
  latitude(REGIONAL_SETTINGS_DEFAULT_LATITUDE),
  longitude(REGIONAL_SETTINGS_DEFAULT_LONGITUDE),
  useMetricUnits(REGIONAL_SETTINGS_DEFAULT_USE_METRICS),
  use24Hours(REGIONAL_SETTINGS_DEFAULT_USE_24HOURS),
  useYMDFormat(REGIONAL_SETTINGS_DEFAULT_USE_YMD),
  forceEngMessages(false),
  updatedParts(0)
  {
}

int RegionalSettings::save(JsonObject& root) {
    root[F("detectAutomatically")] = detectAutomatically;
    root[F("location")] = location;
    root[F("language")] = language;
    root[F("utcOffset")] = utcOffset;
    root[F("latitude")] = latitude;
    root[F("longitude")] = longitude;
    root[F("useMetricUnits")] = useMetricUnits;
    root[F("use24Hours")] = use24Hours;
    root[F("useYMDFormat")] = useYMDFormat;

    print(F("Save Regional settings"));
    return 0;
}

int RegionalSettings::load(JsonObject& root) {
  detectAutomatically = root[F("detectAutomatically")];
  location = root[F("location")].as<const char *>();
  language = root[F("language")].as<const char *>();
  utcOffset = root[F("utcOffset")];
  latitude = root[F("latitude")];
  longitude = root[F("longitude")];
  useMetricUnits = root[F("useMetricUnits")];
  use24Hours = root[F("use24Hours")];
  useYMDFormat = root[F("useYMDFormat")];
  forceEngMessages = nonLatin2Eng( language.c_str());

  print(F("Load Regional settings"));
  return 0;
}

RegionalSettingsEndpoint::RegionalSettingsEndpoint(AsyncWebServer* pServer, FSPersistence *pPersistence, RegionalSettings *pSettings):
    SettingsEndpoint(pServer, F(REGIONAL_SETTINGS_ENDPOINT_PATH), pPersistence, pSettings,
    [this](Settings */*pSettings*/, JsonObject /*jsonObject*/) { //fetchManipulator
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      RegionalSettings *regSettings = (RegionalSettings *)pSettings;
      regSettings->updatedParts = 0;
      
      if(regSettings->detectAutomatically != jsonObject[F("detectAutomatically")]) {
         regSettings->updatedParts |= RegionalSettingsParts::DetectAutomatically;
      }

      if(!jsonObject[F("detectAutomatically")] ) {
        if(jsonObject[F("location")] != regSettings->location) {
          regSettings->updatedParts |= RegionalSettingsParts::City;
        }

        if(jsonObject[F("language")] != regSettings->language) {
          regSettings->updatedParts |= RegionalSettingsParts::Language;
        }

        if(jsonObject[F("utcOffset")] != regSettings->utcOffset) {
          regSettings->updatedParts |= RegionalSettingsParts::UtcOffset;
        }

        if(jsonObject[F("latitude")] != regSettings->latitude 
          || jsonObject[F("longitude")] != regSettings->longitude) {
          regSettings->updatedParts |= RegionalSettingsParts::LocationCoords;
        }

        if(jsonObject[F("useMetricUnits")] != regSettings->useMetricUnits 
          || jsonObject[F("use24Hours")] != regSettings->use24Hours
          || jsonObject[F("useYMDFormat")] != regSettings->useYMDFormat) {
            regSettings->updatedParts |= RegionalSettingsParts::RegionParams;
          }
      }
      Serial.printf_P(PSTR("Updated parts: %u\n"),regSettings->updatedParts);
    }) {
}
