#include "InfluxDBHelper.h"
#include <InfluxDbCloud.h>
#include "Tools.h"

extern int tempHistory[90];

static void writeResponseData(JsonObject &root,  ValidationStatus code, const char *message = nullptr);

InfluxDBHelper::InfluxDBHelper():_sensor("environment") {

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


// load temperature from InfluxDB - the last 90 minutes and the selected device
void InfluxDBHelper::loadTempHistory( const String &deviceID, bool metric) {
  if(!_client || !_client->getServerUrl().length()) {
    return;
  }
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
  Serial.printf_P(PSTR("Validating InfluxDB params: %s, %s, %s, %s\n"),serverUrl.c_str(), org.c_str(), bucket.c_str(), authToken.c_str());
  InfluxDBClient client(serverUrl.c_str(), org.c_str(), bucket.c_str(), authToken.c_str(), InfluxDbCloud2CACert );
  FluxQueryResult res = client.query("buckets()");
  while(res.next()) yield();
  res.close();
  if(res.getError().length()) {
    Serial.printf_P(PSTR("    error: %s\n"),res.getError().c_str());
  }
  return res.getError();
}

void printInfluxDBettings(String prefix, InfluxDBSettings *s) {
    Serial.print(prefix);
    Serial.print(F(" server: "));Serial.print(s->serverURL);
    Serial.print(F(", token: "));Serial.print(s->authorizationToken);
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
    root[F("token")] = authorizationToken;
    root[F("org")] = org;
    root[F("bucket")] = bucket;
    root[F("writeInterval")] = writeInterval;
    printInfluxDBettings(F("InfluxDBSettings::Save"), this);
    return 0;
}

int InfluxDBSettings::load(JsonObject& root) {
    serverURL = root[F("server")].as<const char *>();
    authorizationToken = root[F("token")].as<const char *>();
    org = root[F("org")].as<const char *>();
    bucket = root[F("bucket")].as<const char *>() ;
    writeInterval = root[F("writeInterval")]; 
    printInfluxDBettings(F("InfluxDBSettings::Load"), this);
    return 1;
}

InfluxDBValidateParamsEndpoint::InfluxDBValidateParamsEndpoint(AsyncWebServer* server):
  _validationSettings(nullptr),
  _status(ValidationStatus::Idle),
  _validateHandler(VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH, 
                    std::bind(&InfluxDBValidateParamsEndpoint::validateParams, this, std::placeholders::_1, std::placeholders::_2),
                    DEFAULT_BUFFER_SIZE) {
  _validateHandler.setMethod(HTTP_POST);
  server->addHandler(&_validateHandler);
  server->on(VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH, HTTP_GET, std::bind(&InfluxDBValidateParamsEndpoint::checkStatus, this, std::placeholders::_1));
}

static void writeResponseData(JsonObject &root,  ValidationStatus code, const char *message) {
  root[F("status")] = code;
  if(message) {
    root[F("message")] = message;
  }
}

void InfluxDBValidateParamsEndpoint::validateParams(AsyncWebServerRequest* request, JsonVariant& json) {
    if (!json.is<JsonObject>()) {
        request->send(400);
        return;
    }
    ValidationStatus responseState = _status;
    if(_status == ValidationStatus::Idle || _status >= ValidationStatus::Finished) {
      if(_validationSettings) {
        delete _validationSettings;
      }
      JsonObject jsonObject = json.as<JsonObject>();
      _validationSettings = new InfluxDBSettings;
      _validationSettings->load(jsonObject);
      request->onDisconnect([this]() {
        _status = ValidationStatus::StartRequest;
      });
      responseState = ValidationStatus::StartRequest;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    JsonObject responseJson = response->getRoot().to<JsonObject>();    
    writeResponseData(responseJson, responseState);
    response->setLength();
    response->setCode(200); 
    request->send(response);

}



void InfluxDBValidateParamsEndpoint::loop() {
  if(_status == ValidationStatus::StartRequest) {
    _status = ValidationStatus::Running;
    _error = InfluxDBHelper::validateConnection(_validationSettings->serverURL, _validationSettings->org, _validationSettings->bucket, _validationSettings->authorizationToken);
    delete _validationSettings;
    _validationSettings = nullptr;
    if(!_error.length()) {
      _status = ValidationStatus::Finished;
    } else {
      _status = ValidationStatus::Error;
    }
  }
}

void  InfluxDBValidateParamsEndpoint::checkStatus(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject jsonObject = response->getRoot().to<JsonObject>();
  if (_status < ValidationStatus::Finished) {
    writeResponseData(jsonObject, _status);
  } else {
    if(_status == ValidationStatus::Error && _error.length() && _error[0] == '{') {
      DynamicJsonDocument doc(DEFAULT_BUFFER_SIZE);
      deserializeJson(doc, _error.c_str());
      _error = doc[F("message")].as<const char *>();
    }
    writeResponseData(jsonObject,_status, _status == ValidationStatus::Error?_error.c_str():nullptr);
    request->onDisconnect([this]() {
      _status = ValidationStatus::Idle;
      _error = (char *)nullptr;
    });
  }
  response->setLength();
  response->setCode(200);
  request->send(response);
}
