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
  forceEngMessages(false)
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
