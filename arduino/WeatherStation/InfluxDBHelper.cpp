#include "InfluxDBHelper.h"
#include <InfluxDbCloud.h>
#include "Tools.h"
#include "Version.h"
#include "FSPersistance.h"

extern int tempHistory[90];
const char *Token PROGMEM = "token";

InfluxDBHelper::InfluxDBHelper():_sensor("environment") {

}

void InfluxDBHelper::release() {
  if(_client) {
    delete _client;
    _client = nullptr;
  }
}

void InfluxDBHelper::begin(InfluxDBSettings *settings) {
  if(_client) {
    delete _client;
  }
  _settings = settings;
  _client = new InfluxDBClient(_settings->serverURL.c_str(), _settings->org.c_str(), _settings->bucket.c_str(), _settings->authorizationToken.c_str(), InfluxDbCloud2CACert);
  _client->setHTTPOptions(HTTPOptions().connectionReuse(_settings->writeInterval == 1));  
}


void InfluxDBHelper::update( bool firstStart, const String &deviceID,  const String &wifi, const String &version, const String &location, bool metric) {
  if(!_client || !_client->getServerUrl().length()) {
    return;
  }
  _sensor.clearTags();
  _sensor.addTag(String(F("clientId")), deviceID);
  _sensor.addTag(String(F("Device")), String(F("WS-ESP8266")));
  _sensor.addTag(String(F("Version")), version);
  _sensor.addTag(String(F("Location")), location);
  _sensor.addTag(String(F("WiFi")), wifi);
  _sensor.addTag(String(F("TemperatureSensor")), String(F("DHT11")));
  _sensor.addTag(String(F("HumiditySensor")), String(F("DHT11")));
  _sensor.addTag(String(F("TemperatureUnit")), String(F("C")));
  if(firstStart) {
    loadTempHistory( deviceID, metric);
  }
}


void InfluxDBHelper::write( float temp, float hum, const float lat, const float lon) {
  if(!_client || !_client->getServerUrl().length()) {
    return;
  }

  _sensor.clearFields();
  // Report temperature and humidity
  _sensor.addField(String(F("Temperature")), temp);
  _sensor.addField(String(F("Humidity")), hum);
  _sensor.addField(String(F("lat")), lat, 6);
  _sensor.addField(String(F("lon")), lon, 6);

   // Print what are we exactly writing
  Serial.print(F("Writing: "));
  Serial.println(_client->pointToLineProtocol(_sensor));

  // Write point
  if (!_client->writePoint(_sensor)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
  }
}

void InfluxDBHelper::writeStatus(const String &resetReason) {
  if(!_client || !_client->getServerUrl().length()) {
    return;
  }

  Point status("device_status");
  status.addTag(F("clientId"), getDeviceID());
  status.addTag(F("device"), String(F("WS-ESP8266")));
  status.addTag(F("version"), VERSION);
  if(resetReason.length()) {
    status.addTag(F("reset_reason"), resetReason);
  }
  status.addField(F("free_heap"), ESP.getFreeHeap());
  status.addField(F("max_alloc_heap"), ESP.getMaxFreeBlockSize());
  status.addField(F("heap_fragmentation"), ESP.getHeapFragmentation());
  status.addField(F("uptime"), millis()/1000.0);
  Serial.print(F("Writing status: "));
  Serial.println(_client->pointToLineProtocol(status));
  if (!_client->writePoint(status)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client->getLastErrorMessage());
  }
}

// load temperature from InfluxDB - the last 90 minutes and the selected device
void InfluxDBHelper::loadTempHistory( const String &deviceID, bool metric) {
  if(!_client || !_client->getServerUrl().length()) {
    return;
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

  // Check if there was an error
  if(result.getError().length() > 0) {
    Serial.print(F("Query error: "));
    Serial.println(result.getError());
  }
  result.close();
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

void printInfluxDBettings(String prefix, InfluxDBSettings *s) {
    Serial.print(prefix);
    Serial.print(F(" server: "));Serial.print(s->serverURL);
    //Serial.print(F(", token: "));Serial.print(s->authorizationToken);
    Serial.print(F(", org: "));Serial.print(s->org);
    Serial.print(F(", bucket: "));Serial.print(s->bucket);
    Serial.print(F(", writeInterval: "));Serial.print(s->writeInterval);
    Serial.println();
}
 
InfluxDBSettings::InfluxDBSettings():
  bucket(INFLUXDB_DEFAULT_BUCKET),
  writeInterval(INFLUXDB_DEFAULT_WRITE_INTERVAL) {

}

int InfluxDBSettings::save(JsonObject& root) {
    root[F("server")] = serverURL;
    root[FPSTR(Token)] = authorizationToken;
    root[F("org")] = org;
    root[F("bucket")] = bucket;
    root[F("writeInterval")] = writeInterval;
    printInfluxDBettings(F("InfluxDBSettings::save:"), this);
    return 0;
}

int InfluxDBSettings::load(JsonObject& root) {
    serverURL = root[F("server")].as<const char *>();
    authorizationToken = root[FPSTR(Token)].as<const char *>();
    org = root[F("org")].as<const char *>();
    bucket = root[F("bucket")].as<const char *>() ;
    writeInterval = root[F("writeInterval")]; 
    printInfluxDBettings(F("InfluxDBSettings::load:"), this);
    return 1;
}


// ****************** InfluxDBSettingsEndpoint ***************************

String obfuscateToken(const String &token) {
  String authToken;
  authToken.reserve(15);
  authToken = token.substring(0,4);
  authToken += "******";
  authToken += token.substring(token.length()-4);
  return authToken;
}

InfluxDBSettingsEndpoint::InfluxDBSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, InfluxDBSettings *pSettings):
    SettingsEndpoint(pServer, INFLUXDB_SETTINGS_ENDPOINT_PATH, pPersistence, pSettings, 
    [](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      InfluxDBSettings *idbSettings = (InfluxDBSettings *)pSettings;
      if(idbSettings->authorizationToken.length()>4) {
        jsonObject[FPSTR(Token)] = obfuscateToken(idbSettings->authorizationToken);
      }
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      const char *authToken = jsonObject[FPSTR(Token)].as<const char *>();
      if(strchr(authToken,'*')) {
        //DynamicJsonDocument doc(DEFAULT_BUFFER_SIZE);
        //JsonObject tmpObject = doc.to<JsonObject>();
        //_settings->save(tmpObject);
        //jsonObject[FPSTR(Token)] = tmpObject[FPSTR(Token)].as<const char *>();
        InfluxDBSettings *idbSettings = (InfluxDBSettings *)pSettings;
        jsonObject[FPSTR(Token)] = idbSettings->authorizationToken;
      }
    }) {
    
}

// ****************** InfluxDBValidateParamsEndpoint ***************************

InfluxDBValidateParamsEndpoint::InfluxDBValidateParamsEndpoint(AsyncWebServer* server, InfluxDBHelper *helper):
  ValidateParamsEndpoint(server, VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH),
  _validationSettings(nullptr),
  _helper(helper)
  {

}

void InfluxDBValidateParamsEndpoint::saveParams(JsonVariant& json) {
 if(_validationSettings) {
    delete _validationSettings;
  }
  JsonObject jsonObject = json.as<JsonObject>();
  _validationSettings = new InfluxDBSettings;
  _validationSettings->load(jsonObject);
  const char *authToken = jsonObject[FPSTR(Token)].as<const char *>();
  if(strchr(authToken,'*')) {
    _validationSettings->authorizationToken = _helper->settings()->authorizationToken;
  }
}

void InfluxDBValidateParamsEndpoint::runValidation() {
  _error = _helper->validateConnection(_validationSettings->serverURL, _validationSettings->org, _validationSettings->bucket, _validationSettings->authorizationToken);
  delete _validationSettings;
  _validationSettings = nullptr;
}

