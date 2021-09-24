#include "RegionalSettings.h"
#include <OpenWeatherMapCurrent.h>

void printRegionalSettings(String prefix, RegionalSettings *s) {
    Serial.print(prefix);
    Serial.print(F(" detectAutomatically: "));Serial.print(s->detectAutomatically);
    Serial.print(F(", location: "));Serial.print(s->location);
    Serial.print(F(", language: "));Serial.print(s->language);
    Serial.print(F(", utcOffset: "));Serial.print(s->utcOffset);
    Serial.print(F(", latitude: "));Serial.print(s->latitude);
    Serial.print(F(", longitude: "));Serial.print(s->longitude);
    Serial.print(F(", useMetricUnits: "));Serial.print(s->useMetricUnits);
    Serial.print(F(", use24Hours: "));Serial.print(s->use24Hours);
    Serial.print(F(", useYMDFormat: "));Serial.print(s->useYMDFormat);
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
  useYMDFormat(REGIONAL_SETTINGS_DEFAULT_USE_YMD)
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

    printRegionalSettings("Save Regional settings", this);
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

  printRegionalSettings("Load Regional settings", this);
  return 0;
}

RegionalSettingsValidateEndpoint::RegionalSettingsValidateEndpoint(AsyncWebServer* server, tConfig *pConf):
  ValidateParamsEndpoint(server, REGIONAL_SETTINGS_VALIDATE_ENDPOINT_PATH),
  _pConf(pConf) {}

void RegionalSettingsValidateEndpoint::saveParams(JsonVariant& json) {
 if(_pSettings) {
    delete _pSettings;
  }
  JsonObject jsonObject = json.as<JsonObject>();
  _pSettings = new RegionalSettings;
  _pSettings->load(jsonObject);
}

void RegionalSettingsValidateEndpoint::runValidation() {
  OpenWeatherMapCurrentData data;
  OpenWeatherMapCurrent client;
  Serial.printf_P(PSTR(" Running regional validation: location: %s, lang %s, metrics: %d\n"),_pSettings->location.c_str(),_pSettings->language.c_str(), _pSettings->useMetricUnits);
  client.setLanguage(_pSettings->language);
  client.setMetric(_pSettings->useMetricUnits);
  client.updateCurrent(&data, _pConf->openweatherApiKey, _pSettings->location);
  if(!data.cityName.length()) {
    _error = F("City not found");
  }
  delete _pSettings;
  _pSettings = nullptr;
}
