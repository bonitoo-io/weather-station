#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "InfluxDBHelper.h"

extern int tempHistory[90];

Point sensor("environment"); // Data point
InfluxDBClient influxDBClient;

void setupInfluxDB(InfluxDBSettings *settings) {
  influxDBClient.setConnectionParams(settings->serverURL.c_str(), settings->org.c_str(), settings->bucket.c_str(), settings->authorizationToken.c_str(), InfluxDbCloud2CACert);
  HTTPOptions htOpt;
  htOpt.connectionReuse(settings->writeInterval == 1);
  influxDBClient.setHTTPOptions(htOpt);  
}

// load temperature from InfluxDB - the last 90 minutes and the selected device
void loadTempHistory( const String &bucket, const String &deviceID) { 
  if(!influxDBClient.getServerUrl().length()) {
    return;
  }
  String query = String(F("from(bucket: \"")) + bucket + String(F("\") |> range(start: -90m) |> filter(fn: (r) => r[\"clientId\"] == \"")) + deviceID +  String(F("\")"
  "|> filter(fn: (r) => r[\"_measurement\"] == \"environment\") |> filter(fn: (r) => r[\"_field\"] == \"Temperature\")"
  "|> drop(columns: [\"_start\", \"_stop\", \"_time\", \"Device\", \"HumiditySensor\", \"Location\", \"TemperatureSensor\", \"_measurement\", \"clientId\", \"Version\", \"WiFi\", \"_field\"]) |> limit(n:90)"));

  Serial.print(F("Querying: "));
  Serial.println(query);

  unsigned int i = 0;
  FluxQueryResult result = influxDBClient.query(query);
  while (result.next()) {
    double value = result.getValueByName(String(F("_value"))).getDouble();
    tempHistory[ i] = round( value * 10);
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

void updateInfluxDB( bool firstStart, const String &deviceID, const String &bucket, const String &wifi, const String &version, const String &location) {
  if(!influxDBClient.getServerUrl().length()) {
    return;
  }
  sensor.clearTags();
  sensor.addTag(String(F("clientId")), deviceID);
  sensor.addTag(String(F("Device")), String(F("WS-ESP8266")));
  sensor.addTag(String(F("Version")), version);
  sensor.addTag(String(F("Location")), location);
  sensor.addTag(String(F("WiFi")), wifi);
  sensor.addTag(String(F("TemperatureSensor")), String(F("DHT11")));
  sensor.addTag(String(F("HumiditySensor")), String(F("DHT11")));

  loadTempHistory( bucket, deviceID);
}

bool errorInfluxDB() {
  return (influxDBClient.getServerUrl().length() == 0) || (influxDBClient.getLastErrorMessage().length() != 0);
}

String errorInfluxDBMsg() {
  return influxDBClient.getLastErrorMessage();
}

void writeInfluxDB( float temp, float hum, const float lat, const float lon) {
  if(!influxDBClient.getServerUrl().length()) {
    return;
  }
  if (influxDBClient.getLastErrorMessage().length() != 0)
    influxDBClient.validateConnection();

  sensor.clearFields();
  // Report temperature and humidity
  sensor.addField(String(F("Temperature")), temp);
  sensor.addField(String(F("Humidity")), hum);
  sensor.addField(String(F("Lat")), lat, 6);
  sensor.addField(String(F("Lon")), lon, 6);

  // Print what are we exactly writing
  Serial.print(F("Writing: "));
  Serial.println(influxDBClient.pointToLineProtocol(sensor));

  // Write point
  if (!influxDBClient.writePoint(sensor)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(influxDBClient.getLastErrorMessage());
  }
}

void printInfluxDBettings(String prefix, InfluxDBSettings *s) {
    Serial.print(prefix);
    Serial.print(" server: ");Serial.print(s->serverURL);
    Serial.print(", token: ");Serial.print(s->authorizationToken);
    Serial.print(", org: ");Serial.print(s->org);
    Serial.print(", bucket: ");Serial.print(s->bucket);
    Serial.print(", writeInterval: ");Serial.print(s->writeInterval);
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
