#include "AdvancedSettings.h"
#include "ScreenCommon.h"
#include "Sensor.h"
#include "WeatherStation.h"
#include "Tools.h"

#define ADVANCED_DEFAUT_UPDATE_INTERVAL 60
//See https://docs.thingpulse.com/how-tos/openweathermap-key/
#define ADVANCED_DEFAUT_OPENWEATHER_API_KEY ""
#define ADVANCED_DEFAUT_NTP_SERVERS "pool.ntp.org,time.nis.gov,time.google.com"
#define ADVANCED_DEFAUT_TEMPERATURE_OFFSET 0.0
#define ADVANCED_DEFAUT_HUMIDITY_OFFSET 0.0
#define ADVANCED_DEFAULT_OWNER F("bonitoo-io")
#define ADVANCED_DEFAULT_REPO F("weather-station")
#define ADVANCED_DEFAULT_BIN_FILE F("ws-firmware-%version%.bin")
#define ADVANCED_DEFAULT_MD5_FILE F("ws-firmware-%version%.md5")
#define ADVANCED_DEFAULT_UPDATETIME 300 //HHMM
#define ADVANCED_DEFAULT_CHECKBETA  false
#define ADVANCED_DEFAULT_VERIFY_CERT  false

#include "custom_dev.h"

const char *OpenweatherApiKeyStr PROGMEM = "openWeatherAPIKey";

static uint16_t getDefaultUpdateTime() {
  uint8_t mac[6];
  wifi_get_macaddr(STATION_IF, mac);
  return ADVANCED_DEFAULT_UPDATETIME + mac[5];
}

AdvancedSettings::AdvancedSettings():
  updateDataInterval(ADVANCED_DEFAUT_UPDATE_INTERVAL),
  ntpServers(F(ADVANCED_DEFAUT_NTP_SERVERS)),
  owner(ADVANCED_DEFAULT_OWNER),
  repo(ADVANCED_DEFAULT_REPO),
  binFile(ADVANCED_DEFAULT_BIN_FILE),
  md5File(ADVANCED_DEFAULT_MD5_FILE),
  checkBeta(ADVANCED_DEFAULT_CHECKBETA),
  verifyCert(ADVANCED_DEFAULT_VERIFY_CERT)  {
  setUpdateTime(getDefaultUpdateTime());
  DECLARE_ENCRYPT_STR( owKeyStr, ADVANCED_DEFAUT_OPENWEATHER_API_KEY);
  openWeatherAPIKey = GET_ENCRYPT_STR(owKeyStr);
}

void AdvancedSettings::begin() {
  _eepromData.begin();
  if(_eepromData.read() != 0) {  //read error? load default values
    _eepromData.setTempOffsetRaw(ADVANCED_DEFAUT_TEMPERATURE_OFFSET);
    _eepromData.setHumOffsetRaw(ADVANCED_DEFAUT_HUMIDITY_OFFSET);
  }
}

void AdvancedSettings::setUpdateTime(uint16_t time) {
  uint8_t h = time/100;
  uint8_t m = time%100;
  if(m >= 60) {
    ++h;
    m = m - 60;
    time = h*100+m;
  }
  updateTime = time;
}

void AdvancedSettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" updateDataInterval: "));Serial.print(updateDataInterval);
    Serial.print(F(", openWeatherAPIKey: "));Serial.print(obfuscateToken(openWeatherAPIKey));
    Serial.print(F(", ntpServers: "));Serial.print(ntpServers);
    Serial.print(F(", tempOffset: "));Serial.print(_eepromData.getTempOffsetRaw());
    Serial.print(F(", humOffset: "));Serial.print(_eepromData.getHumOffsetRaw());
    Serial.print(F(", owner: "));Serial.print(owner);
    Serial.print(F(", repo: "));Serial.print(repo);
    Serial.print(F(", binFile: "));Serial.print(binFile);
    Serial.print(F(", md5File: "));Serial.print(md5File);
    Serial.print(F(", updateTime: "));Serial.print(updateTime);
    Serial.print(F(", checkBeta: "));Serial.print(checkBeta);
    Serial.print(F(", verifyCert: "));Serial.print(verifyCert);
    Serial.println();
}

int AdvancedSettings::save(JsonObject& root) {
    root[F("updateDataInterval")] = updateDataInterval;
    root[FPSTR(OpenweatherApiKeyStr)] = openWeatherAPIKey;
    root[F("ntpServers")] = ntpServers;
    root[F("tempOffset")] = getTempOffset();
    root[F("humOffset")] = getHumOffset();
    root[F("owner")] = owner;
    root[F("repo")] = repo;
    root[F("binFile")] = binFile;
    root[F("md5File")] = md5File;
    root[F("updateTime")] = updateTime;
    root[F("checkBeta")] = checkBeta;
    root[F("verifyCert")] = verifyCert;

    print(F("Save Advanced settings"));
    return 0;
}

int AdvancedSettings::load(JsonObject& root) {
  updateDataInterval = root[F("updateDataInterval")];
  openWeatherAPIKey = root[FPSTR(OpenweatherApiKeyStr)].as<const char *>();
  ntpServers = root[F("ntpServers")].as<const char *>();
  owner = root[F("owner")] | String(ADVANCED_DEFAULT_OWNER);
  repo = root[F("repo")] | String(ADVANCED_DEFAULT_REPO);
  binFile = root[F("binFile")] | String(ADVANCED_DEFAULT_BIN_FILE);
  md5File = root[F("md5File")] | String(ADVANCED_DEFAULT_MD5_FILE);
  setUpdateTime(root[F("updateTime")] | getDefaultUpdateTime());
  checkBeta = root[F("checkBeta")];
  verifyCert = root[F("verifyCert")] | ADVANCED_DEFAULT_VERIFY_CERT;

  print(F("Load Advanced settings"));
  return 0;
}

void AdvancedSettings::updateEEPROMData( float tempOffset, float humOffset) {
  setTempOffset(tempOffset);
  setHumOffset(humOffset);
  _eepromData.write();
}

float AdvancedSettings::getTempOffsetF() {
  return _eepromData.getTempOffsetRaw();
}

float AdvancedSettings::getTempOffset() {
  return station.getRegionalSettings()->useMetricUnits ? getTempOffsetF() / 9.0 * 5.0 : getTempOffsetF(); //convert from F to C if needed
}

float AdvancedSettings::getHumOffset() {
  return _eepromData.getHumOffsetRaw();
}

void AdvancedSettings::setTempOffset( float tempOffset) {
  _eepromData.setTempOffsetRaw( station.getRegionalSettings()->useMetricUnits ? tempOffset * 9.0 / 5.0 : tempOffset); //convert from F to C if needed
}

void AdvancedSettings::setHumOffset( float humOffset) {
  _eepromData.setHumOffsetRaw(humOffset);
}

AdvancedSettingsEndpoint::AdvancedSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, AdvancedSettings *pSettings, RegionalSettings *pRegionalSettings):
    SettingsEndpoint(pServer, F(ADVANCED_SETTINGS_ENDPOINT_PATH), pPersistence, pSettings,
    [this](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      AdvancedSettings *advSettings = (AdvancedSettings *)pSettings;
      if(advSettings->openWeatherAPIKey.length()>4) {
        jsonObject[FPSTR(OpenweatherApiKeyStr)] = obfuscateToken(advSettings->openWeatherAPIKey);
      }
      jsonObject[F("use24Hours")] = _pRegionalSettings->use24Hours;
      jsonObject[F("useMetric")] = _pRegionalSettings->useMetricUnits;
      jsonObject[F("actualTemp")] = _pRegionalSettings->useMetricUnits ? Sensor::tempF2C(pSensor->getTempF(true)) : pSensor->getTempF(true);
      jsonObject[F("actualHum")] = pSensor->getHum(true);
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      const char *key = jsonObject[FPSTR(OpenweatherApiKeyStr)].as<const char *>();
      AdvancedSettings *advSettings = (AdvancedSettings *)pSettings;
      if(strstr(key, ReplaceMark)) {
        jsonObject[FPSTR(OpenweatherApiKeyStr)] = advSettings->openWeatherAPIKey;
      }
      advSettings->updateEEPROMData( jsonObject[F("tempOffset")], jsonObject[F("humOffset")]);
    }), _pRegionalSettings(pRegionalSettings) {
}
