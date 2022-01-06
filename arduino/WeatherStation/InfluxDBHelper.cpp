#include "InfluxDBHelper.h"
#include <InfluxDbCloud.h>
#include "Tools.h"
#include "Version.h"
#include "FSPersistance.h"
#include "ServiceState.h"
#include "WeatherStation.h"

extern int tempHistory[90];
const char *Token PROGMEM = "token";

#define LOCK() { uint16_t _lc=3000; while(_lock) { delay(1); if(!--_lc) {Serial.println(F("Lock timeout!"));break; } } _lock = 1; }
#define UNLOCK()  { _lock = 0; } 

InfluxDBHelper::InfluxDBHelper():_sensor("environment") {

}

bool InfluxDBHelper::release() {
  if(_lock) {
    return false;
  }
  if(_client) {
    delete _client;
    _client = nullptr;
    _wasReleased = true;
  }
  return true;
}

void InfluxDBHelper::begin(InfluxDBSettings *settings) {
  if(_client) {
    delete _client;
  }
  _settings = settings;
  _client = new InfluxDBClient(_settings->serverURL.c_str(), _settings->org.c_str(), _settings->bucket.c_str(), _settings->authorizationToken.c_str(), InfluxDbCloud2CACert);
  _client->setHTTPOptions(HTTPOptions().connectionReuse(_settings->writeInterval == 1).httpReadTimeout(20000));  
  _wasReleased = false;
}


void InfluxDBHelper::update( bool firstStart, const String &deviceID,  const String &wifi, const String &version, const String &location, bool metric) {
  if(!_client || !_client->getServerUrl().length()) {
    return;
  }
  
  LOCK();
  
  WriteOptions wo;
  wo.batchSize(2).bufferSize(4).maxRetryAttempts(0);
  wo.addDefaultTag(String(F("clientId")), deviceID);
  wo.addDefaultTag(String(F("Device")), String(F("WS-ESP8266")));
  wo.addDefaultTag(String(F("Version")), version);
  wo.addDefaultTag(String(F("Location")), location);
  wo.addDefaultTag(String(F("WiFi")), wifi);
  _client->setWriteOptions(wo);

  _sensor.clearTags();
  _sensor.addTag(String(F("TemperatureSensor")), String(F("DHT11")));
  _sensor.addTag(String(F("HumiditySensor")), String(F("DHT11")));
  _sensor.addTag(String(F("TemperatureUnit")), String(F("C")));
  if(firstStart) {
    loadTempHistory( deviceID, metric);
  }
  UNLOCK();
}


bool InfluxDBHelper::write( float temp, float hum, const float lat, const float lon) {
  if(!_client || !_client->getServerUrl().length()) {
    return false;
  }
  LOCK();
  _sensor.clearFields();
  // Report temperature and humidity
  _sensor.addField(String(F("Temperature")), temp);
  _sensor.addField(String(F("Humidity")), hum);
  _sensor.addField(String(F("lat")), lat, 6);
  _sensor.addField(String(F("lon")), lon, 6);

   // Print what are we exactly writing
  Serial.print(F("Writing: "));
  Serial.println(_client->pointToLineProtocol(_sensor));
  _client->writePoint(_sensor);
  // Write point
  bool res = true;
  if (!_client->flushBuffer()) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
    res = false;
  }
  UNLOCK();
  return res;
}

bool InfluxDBHelper::writeStatus() {
  if(!_client || !_client->getServerUrl().length()) {
    return false;
  }
  LOCK();
  Point status("device_status");
  status.addField(F("free_heap"), ESP.getFreeHeap());
  status.addField(F("max_alloc_heap"), ESP.getMaxFreeBlockSize());
  status.addField(F("heap_fragmentation"), ESP.getHeapFragmentation());
  status.addField(F("uptime"), millis()/1000.0);
  status.addField(F("wifi_disconnects"), station.getWifiManager()->getDisconnectsCount());
  
  Serial.print(F("Writing device status: "));
  Serial.println(_client->pointToLineProtocol(status));
  _client->writePoint(status);

  // Write all statuses except write
  for(uint8_t i = 0; i < SyncServices::ServiceDBWriteStatus; i++) {
    Point *point = ServicesTracker.serviceStatisticToPoint((SyncServices)i);
    Serial.print(F("Writing service status: "));
    Serial.println(_client->pointToLineProtocol(*point));
    _client->writePoint(*point);
    delete point;
  }

  if(_pResetInfo) {
    writeResetInfo();
  }
  bool res = true;
  if (!_client->flushBuffer()) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
    res = false;
  }
  UNLOCK();
  return res;
}

void InfluxDBHelper::registerResetInfo(const String &resetReason) {
  if(_pResetInfo) {
    delete _pResetInfo;
  }
  _pResetInfo = new Point("device_status");
  _pResetInfo->addTag(F("reset_reason"), resetReason);
  _pResetInfo->addField(F("free_heap"), ESP.getFreeHeap());
  _pResetInfo->addField(F("max_alloc_heap"), ESP.getMaxFreeBlockSize());
  _pResetInfo->addField(F("heap_fragmentation"), ESP.getHeapFragmentation());
  for(uint8_t i = 0; i < SyncServices::ServiceLastMark; i++) {
    ServiceStatistic &stat = ServicesTracker.getServiceStatics((SyncServices)i);
    if(stat.state == ServiceState::SyncStarted) {
      _pResetInfo->addTag(F("crashed"), getServiceName((SyncServices)i));
      ServicesTracker.serviceStatisticToPoint((SyncServices)i, _pResetInfo);
    }
  }
}

bool InfluxDBHelper::writeResetInfo() {
  if(!_client || !_client->getServerUrl().length() || !_pResetInfo) {
    return false;
  }
  Serial.print(F("Writing reset info: "));
  Serial.println(_client->pointToLineProtocol(*_pResetInfo));
  bool res = true;
  if (!_client->writePoint(*_pResetInfo)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
    res = false;
  } else {
    delete _pResetInfo;
    _pResetInfo = nullptr;
  }
  return res;
}

// load temperature from InfluxDB - the last 90 minutes and the selected device
bool InfluxDBHelper::loadTempHistory( const String &deviceID, bool metric) {
  if(!_client || !_client->getServerUrl().length()) {
    return false;
  }
  // TODO: optimize to use reserve() and subsequent concatenation
  String query = String(F("from(bucket: \"")) + _settings->bucket + String(F("\") |> range(start: -90m) |> filter(fn: (r) => r[\"clientId\"] == \"")) + deviceID +  String(F("\")"
  "|> filter(fn: (r) => r[\"_measurement\"] == \"environment\") |> filter(fn: (r) => r[\"_field\"] == \"Temperature\")"
  "|> drop(columns: [\"_start\", \"_stop\", \"_time\", \"Device\", \"HumiditySensor\", \"Location\", \"TemperatureSensor\", \"_measurement\", \"clientId\", \"Version\", \"WiFi\", \"_field\"]) |> limit(n:90)"));

  Serial.print(F("Querying: "));
  Serial.println(query);

  unsigned int i = 0;
  FluxQueryResult result = _client->query(query);
  while (result.next()) {
    float value = result.getValueByName(String(F("_value"))).getDouble();
    tempHistory[ i] = metric ? round( value * 10) : round( convertCtoF( value) * 10);
    i++;
    if (i == 90)
      break;
  }
  bool res = true;
  // Check if there was an error
  if(result.getError().length() > 0) {
    Serial.print(F("Query error: "));
    Serial.println(result.getError());
    res = false;
  }
  result.close();
  return res;
}

String InfluxDBHelper::validateConnection(const String &serverUrl, const String &org, const String &bucket, const String &authToken) {
  release();
  Serial.printf_P(PSTR("Validating InfluxDB params: %s, %s, %s, %s\n"),serverUrl.c_str(), org.c_str(), bucket.c_str(), authToken.c_str());
  InfluxDBClient client(serverUrl.c_str(), org.c_str(), bucket.c_str(), authToken.c_str(), InfluxDbCloud2CACert );
  FluxQueryResult res = client.query("buckets()");
  while(res.next()) yield();
  res.close();
  if(res.getError().length()) {
    Serial.printf_P(PSTR("    error: %s\n"),res.getError().c_str());
  }
  begin(_settings);
  return res.getError();
}

void InfluxDBSettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" server: "));Serial.print(serverURL);
    Serial.print(F(", token: "));Serial.print(obfuscateToken(authorizationToken));
    Serial.print(F(", org: "));Serial.print(org);
    Serial.print(F(", bucket: "));Serial.print(bucket);
    Serial.print(F(", writeInterval: "));Serial.print(writeInterval);
    Serial.println();
}
 
InfluxDBSettings::InfluxDBSettings():
  serverURL(INFLUXDB_DEFAULT_SERVER_URL),
  authorizationToken(INFLUXDB_DEFAULT_TOKEN),
  org(INFLUXDB_DEFAULT_ORG),
  bucket(INFLUXDB_DEFAULT_BUCKET),
  writeInterval(INFLUXDB_DEFAULT_WRITE_INTERVAL) {
}

int InfluxDBSettings::save(JsonObject& root) {
  root[F("server")] = serverURL;
  root[FPSTR(Token)] = authorizationToken;
  root[F("org")] = org;
  root[F("bucket")] = bucket;
  root[F("writeInterval")] = writeInterval;
  print(F("Save InfluxDB params"));
  return 0;
}

int InfluxDBSettings::load(JsonObject& root) {
  serverURL = root[F("server")].as<const char *>();
  authorizationToken = root[FPSTR(Token)].as<const char *>();
  org = root[F("org")].as<const char *>();
  bucket = root[F("bucket")].as<const char *>() ;
  writeInterval = root[F("writeInterval")]; 
  print(F("Load InfluxDB params"));
  return 1;
}


// ****************** InfluxDBSettingsEndpoint ***************************

InfluxDBSettingsEndpoint::InfluxDBSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, InfluxDBSettings *pSettings):
    SettingsEndpoint(pServer, INFLUXDB_SETTINGS_ENDPOINT_PATH, pPersistence, pSettings, 
    [](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      InfluxDBSettings *idbSettings = (InfluxDBSettings *)pSettings;
      if(idbSettings->authorizationToken.length()>4) {
        jsonObject[FPSTR(Token)] = obfuscateToken(idbSettings->authorizationToken);
      }
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      const char *authToken = jsonObject[FPSTR(Token)].as<const char *>();
      if(strstr(authToken, ReplaceMark)) {
        InfluxDBSettings *idbSettings = (InfluxDBSettings *)pSettings;
        jsonObject[FPSTR(Token)] = idbSettings->authorizationToken;
      }
    }) {
    
}

// ****************** InfluxDBValidateParamsEndpoint ***************************

InfluxDBValidateParamsEndpoint::InfluxDBValidateParamsEndpoint(AsyncWebServer *pServer, InfluxDBHelper *pHelper):
  ValidateParamsEndpoint(pServer, VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH),
  _validationSettings(nullptr),
  _pHelper(pHelper) { }

void InfluxDBValidateParamsEndpoint::saveParams(JsonVariant& json) {
 if(_validationSettings) {
    delete _validationSettings;
  }
  JsonObject jsonObject = json.as<JsonObject>();
  _validationSettings = new InfluxDBSettings;
  _validationSettings->load(jsonObject);
  const char *authToken = jsonObject[FPSTR(Token)].as<const char *>();
  if(strstr(authToken,ReplaceMark)) {
    _validationSettings->authorizationToken = _pHelper->settings()->authorizationToken;
  }
}

void InfluxDBValidateParamsEndpoint::runValidation() {
  ServicesTracker.updateServiceState(SyncServices::ServiceDBValidate, ServiceState::SyncStarted);
  ServicesTracker.save();
  _error = _pHelper->validateConnection(_validationSettings->serverURL, _validationSettings->org, _validationSettings->bucket, _validationSettings->authorizationToken);
  if(_error.length()) {
    ServicesTracker.updateServiceState(SyncServices::ServiceDBValidate, ServiceState::SyncFailed);
  } else {
    ServicesTracker.updateServiceState(SyncServices::ServiceDBValidate, ServiceState::SyncOk);
  }
  delete _validationSettings;
  _validationSettings = nullptr;
}

