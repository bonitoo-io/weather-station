#include "InfluxDBHelper.h"
#include <InfluxDbCloud.h>
#include "Tools.h"
#include "Version.h"
#include "FSPersistance.h"
#include "ServiceState.h"
#include "WeatherStation.h"
#include "Sensor.h"

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
  _client = new InfluxDBClient(_settings->serverURL, _settings->org, _settings->bucket, _settings->authorizationToken, InfluxDbCloud2CACert);
  _client->setStreamWrite(true);
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
  wo.addDefaultTag(F("clientId"), deviceID);
  wo.addDefaultTag(F("Device"), F("WS-ESP8266"));
  wo.addDefaultTag(F("Version"), version);
  wo.addDefaultTag(F("Location"), location);
  wo.addDefaultTag(F("WiFi"), wifi);
  _client->setWriteOptions(wo);

  _sensor.clearTags();
  _sensor.addTag(F("TemperatureSensor"), pSensor->getSensorName());
  _sensor.addTag(F("HumiditySensor"), pSensor->getSensorName());
  _sensor.addTag(F("TemperatureUnit"), F("C"));
  if(firstStart) {
    loadTempHistory( deviceID);
  }
  UNLOCK();
}


bool InfluxDBHelper::write( float temp, float hum, const float lat, const float lon) {
  if(!_client || !_client->getServerUrl().length()) {
    return false;
  }
  LOCK();
  Serial.print(F("InfluxDB server connected: "));
  Serial.println(_client->isConnected());

  _sensor.clearFields();
  // Report temperature and humidity
  _sensor.addField(F("Temperature"), temp);
  _sensor.addField(F("Humidity"), hum);
  _sensor.addField(F("Lat"), lat, 6);
  _sensor.addField(F("Lon"), lon, 6);

   // Print what are we exactly writing
  Serial.print(F("Writing: "));
  Serial.println(_client->pointToLineProtocol(_sensor));
  
  // Write point
  bool res = _client->writePoint(_sensor);
  if (!res) {  
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
  }
  
  if ( !_client->flushBuffer()) { 
    Serial.print(F("InfluxDB flush failed: "));
    Serial.println(_client->getLastErrorMessage());
    res = false;
  }

  if (res || (_client->getLastStatusCode() > 0)) { //successful write or some http error code received (skip only IP connection issues)?
    writeSuccess++;
  }
 
  UNLOCK();
  return res;
}

bool InfluxDBHelper::writeStatus() {
  bool res = true;
  if(!_client || !_client->getServerUrl().length()) {
    return false;
  }
  LOCK();
  Serial.print(F("InfluxDB server connected: "));
  Serial.println(_client->isConnected());

  if(_pResetInfo) {
    res = writeResetInfo();
  }

  Point status("device_status");
  status.addField(F("free_heap"), ESP.getFreeHeap());
  status.addField(F("max_alloc_heap"), ESP.getMaxFreeBlockSize());
  status.addField(F("heap_fragmentation"), ESP.getHeapFragmentation());
  status.addField(F("uptime"), millis()/1000.0);
  status.addField(F("wifi_disconnects"), station.getWifiManager()->getDisconnectsCount());
  
  Serial.print(F("Writing device status: "));
  Serial.println(_client->pointToLineProtocol(status));
  
  if (!_client->writePoint(status)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
    res = false;
  }
  
  // Write all statuses except write
  for (uint8_t i = 0; i < SyncServices::ServiceDBWriteStatus; i++) {
    Point *point = ServicesTracker.serviceStatisticToPoint((SyncServices)i);
    Serial.print(F("Writing service status: "));
    Serial.println(_client->pointToLineProtocol(*point));
    if (!_client->writePoint(*point)) {
      Serial.print(F("InfluxDB write failed: "));
      Serial.println(_client->getLastErrorMessage());
      res = false;
    }      
    delete point;
  }

  
  if (!_client->flushBuffer()) {
    Serial.print(F("InfluxDB flush failed: "));
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
  _pResetInfo = new Point(F("device_status"));
  _pResetInfo->addTag(F("reset_reason"), resetReason);
  _pResetInfo->addField(F("free_heap"), ESP.getFreeHeap());
  _pResetInfo->addField(F("max_alloc_heap"), ESP.getMaxFreeBlockSize());
  _pResetInfo->addField(F("heap_fragmentation"), ESP.getHeapFragmentation());
  if(ServicesTracker.getResetCount()> 1) {
    _pResetInfo->addField(F("crash_resets"), ServicesTracker.getResetCount()-1);
  }
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
bool InfluxDBHelper::loadTempHistory( const String &deviceID) {
  if(!_client || !_client->getServerUrl().length()) {
    return false;
  }
  // Prepare query parameters
  QueryParams params;
  params.add(F("bucket"), _settings->bucket);
  params.add(F("deviceID"), deviceID);
  
  String query = F("from(bucket: params.bucket) |> range(start: -90m) |> filter(fn: (r) => r[\"clientId\"] == params.deviceID)"
  "|> filter(fn: (r) => r[\"_measurement\"] == \"environment\") |> filter(fn: (r) => r[\"_field\"] == \"Temperature\")"
  "|> drop(columns: [\"_start\", \"_stop\", \"_time\", \"Device\", \"HumiditySensor\", \"Location\", \"TemperatureSensor\", \"_measurement\", \"clientId\", \"Version\", \"WiFi\", \"_field\"]) |> limit(n:90)");

  Serial.print(F("Querying: "));
  Serial.println(query);

  uint16_t i = 0;
  FluxQueryResult result = _client->query(query, params);
  while (result.next()) {
    float value = result.getValueByName(F("_value")).getDouble();
    pSensor->setHist( i, Sensor::temp2Int( value, true));   //metric = true - we save data into InfluxDB always in C
    //Serial.println( "tempHistory[" + String(i) + "] " + String(value) + "->" + String(pSensor->getHist( i)) + " raw: " + String(pSensor->getRawHist( i)));
    i++;
    if (i == TEMP_HIST_SIZE)  //is the history is full?
      break;
  }
  Serial.print(F("Loaded values: "));
  Serial.println(i);

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
  InfluxDBClient client(serverUrl, org, bucket, authToken, InfluxDbCloud2CACert );
  FluxQueryResult res = client.query("buckets()");
  while(res.next()) yield();
  res.close();
  if(res.getError().length()) {
    Serial.printf_P(PSTR("    error: %s\n"),res.getError().c_str());
  }
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
    SettingsEndpoint(pServer, F(INFLUXDB_SETTINGS_ENDPOINT_PATH), pPersistence, pSettings, 
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
  ValidateParamsEndpoint(pServer, F(VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH)),
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
