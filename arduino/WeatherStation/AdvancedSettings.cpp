#include "AdvancedSettings.h"
#include "ScreenCommon.h"

const char *OpenweatherApiKeyStr PROGMEM = "openWeatherAPIKey";

static uint16_t getDefaultUpdateTime() {
  uint8_t mac[6];
  wifi_get_macaddr(STATION_IF, mac);
  return ADVANCED_DEFAULT_UPDATETIME + mac[5]; 
}

AdvancedSettings::AdvancedSettings():
  updateDataInterval(ADVANCED_DEFAUT_UPDATE_INTERVAL),
  openWeatherAPIKey(F(ADVANCED_DEFAUT_OPENWEATHER_API_KEY)),
  ntpServers(F(ADVANCED_DEFAUT_NTP_SERVERS)),
  tempOffset(ADVANCED_DEFAUT_TEMPERATURE_OFFSET),
  humOffset(ADVANCED_DEFAUT_HUMIDITY_OFFSET),
  owner(ADVANCED_DEFAULT_OWNER),
  repo(ADVANCED_DEFAULT_REPO),
  binFile(ADVANCED_DEFAULT_BIN_FILE),
  md5File(ADVANCED_DEFAULT_MD5_FILE),
  checkBeta(ADVANCED_DEFAULT_CHECKBETA),
  verifyCert(ADVANCED_DEFAULT_VERIFY_CERT)  {
  setUpdateTime(getDefaultUpdateTime());
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
    Serial.print(F(", tempOffset: "));Serial.print(tempOffset);
    Serial.print(F(", humOffset: "));Serial.print(humOffset);
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
    root[F("tempOffset")] = tempOffset;
    root[F("humOffset")] = humOffset;
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
  tempOffset = root[F("tempOffset")];
  humOffset = root[F("humOffset")];
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


AdvancedSettingsEndpoint::AdvancedSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, AdvancedSettings *pSettings, RegionalSettings *pRegionalSettings):
    SettingsEndpoint(pServer, F(ADVANCED_SETTINGS_ENDPOINT_PATH), pPersistence, pSettings, 
    [this](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      AdvancedSettings *advSettings = (AdvancedSettings *)pSettings;
      if(advSettings->openWeatherAPIKey.length()>4) {
        jsonObject[FPSTR(OpenweatherApiKeyStr)] = obfuscateToken(advSettings->openWeatherAPIKey);
      }
      jsonObject[F("use24Hours")] = _pRegionalSettings->use24Hours;
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      const char *key = jsonObject[FPSTR(OpenweatherApiKeyStr)].as<const char *>();
      if(strstr(key, ReplaceMark)) {
        AdvancedSettings *advSettings = (AdvancedSettings *)pSettings;
        jsonObject[FPSTR(OpenweatherApiKeyStr)] = advSettings->openWeatherAPIKey;
      }
    }), _pRegionalSettings(pRegionalSettings) {
    
}

