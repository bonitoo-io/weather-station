#include "InfluxDB.h"

void printInfluxDBettings(String prefix, InfluxDBSettings *s) {
    Serial.print(prefix);
    Serial.print(" server: ");Serial.print(s->serverURL);
    Serial.print(", token: ");Serial.print(s->authorizationToken);
    Serial.print(", org: ");Serial.print(s->org);
    Serial.print(", bucket: ");Serial.print(s->bucket);
    Serial.print(", writeInterval: ");Serial.print(s->writeInterval);
    Serial.println();
}

int InfluxDBSettings::save(JsonObject& root) {
    root[F("server")] = serverURL;
    root[F("token")] = authorizationToken;
    root[F("org")] = org;
    root[F("bucket")] = bucket;
    root[F("writeInterval")] = writeInterval;
    printInfluxDBettings("Save", this);
    return 0;
}

int InfluxDBSettings::load(JsonObject& root) {
    serverURL = root[F("server")].as<const char *>();
    authorizationToken = root[F("token")].as<const char *>();
    org = root[F("org")].as<const char *>();
    bucket = root[F("bucket")] | INFLUXDB_DEFAULT_BUCKET;
    writeInterval = root[F("writeInterval")] | INFLUXDB_DEFAULT_WRITE_INTERVAL;
    printInfluxDBettings("Load", this);
    return 1;
}


InfluxDBWriter::InfluxDBWriter():_sensor("environment") {

}


void InfluxDBWriter::setupInfluxDB( const String &serverUrl, const String &org, const String &bucket, const String &authToken, int refresh_sec) {
  _client.setConnectionParams(serverUrl.c_str(), org.c_str(), bucket.c_str(), authToken.c_str(), InfluxDbCloud2CACert);
  HTTPOptions htOpt;
  htOpt.connectionReuse(refresh_sec <= 60);
  _client.setHTTPOptions(htOpt);  
}


void InfluxDBWriter::updateInfluxDB( bool firstStart, const String &deviceID,  const String &wifi, const String &version, const String &location) {
  // Check server connection
  if (firstStart) {
    _sensor.addTag(String(F("clientId")), deviceID);
    _sensor.addTag(String(F("Device")), String(F("WS-ESP8266")));
    _sensor.addTag(String(F("Version")), version);
    _sensor.addTag(String(F("Location")), location);
    _sensor.addTag(String(F("WiFi")), wifi);
    _sensor.addTag(String(F("TemperatureSensor")), String(F("DHT11")));
    _sensor.addTag(String(F("HumiditySensor")), String(F("DHT11")));

    if (_client.validateConnection()) {
      Serial.print(F("Connected to InfluxDB: "));
      Serial.println(_client.getServerUrl());
    } else {
      Serial.print(F("InfluxDB connection failed: "));
      Serial.println(_client.getLastErrorMessage());
    }
  }
}


 

void InfluxDBWriter::writeInfluxDB( float temp, float hum, const float lat, const float lon) {
  if (_client.getLastErrorMessage().length())
    _client.validateConnection();

  _sensor.clearFields();
  // Report temperature and humidity
  _sensor.addField(String(F("Temperature")), temp);
  _sensor.addField(String(F("Humidity")), hum);
  _sensor.addField(String(F("Lat")), lat, 6);
  _sensor.addField(String(F("Lon")), lon, 6);

  // Print what are we exactly writing
  Serial.print(F("Writing: "));
  Serial.println(_client.pointToLineProtocol(_sensor));

  // Write point
  if (!_client.writePoint(_sensor)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(_client.getLastErrorMessage());
  }
}