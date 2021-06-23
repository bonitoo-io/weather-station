#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "InfluxDBHelper.h"
#include "Tools.h"

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
void loadTempHistory( const String &bucket, const String &deviceID, bool metric) {
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

void updateInfluxDB( bool firstStart, const String &deviceID, const String &bucket, const String &wifi, const String &version, const String &location, bool metric) {
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
  sensor.addTag(String(F("TemperatureUnit")), String(F("C")));
  if(firstStart) {
    loadTempHistory( bucket, deviceID, metric);
  }
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

  sensor.addField(F("Temperature"), temp);
  sensor.addField(F("Humidity"), hum);
  sensor.addField(F("lat"), lat, 6);
  sensor.addField(F("lon"), lon, 6);

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
