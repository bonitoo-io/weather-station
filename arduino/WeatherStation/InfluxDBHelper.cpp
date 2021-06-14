#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

extern int tempHistory[90];

Point sensor("environment"); // Data point
InfluxDBClient influxDBClient;

void setupInfluxDB( const String &serverUrl, const String &org, const String &bucket, const String &authToken, int refresh_sec) {
  influxDBClient.setConnectionParams(serverUrl.c_str(), org.c_str(), bucket.c_str(), authToken.c_str(), InfluxDbCloud2CACert);
  HTTPOptions htOpt;
  htOpt.connectionReuse(refresh_sec <= 60);
  influxDBClient.setHTTPOptions(htOpt);
}

void loadTempHistory( const String &bucket, const String &deviceID) { //load temperature from InfluxDB - the last 90 minutes and the selected device
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
  // Check server connection
  if (firstStart) {
    sensor.addTag(String(F("clientId")), deviceID);
    sensor.addTag(String(F("Device")), String(F("WS-ESP8266")));
    sensor.addTag(String(F("Version")), version);
    sensor.addTag(String(F("Location")), location);
    sensor.addTag(String(F("WiFi")), wifi);
    sensor.addTag(String(F("TemperatureSensor")), String(F("DHT11")));
    sensor.addTag(String(F("HumiditySensor")), String(F("DHT11")));

    if (influxDBClient.validateConnection()) {
      Serial.print(F("Connected to InfluxDB: "));
      Serial.println(influxDBClient.getServerUrl());
      loadTempHistory( bucket, deviceID);
    } else {
      Serial.print(F("InfluxDB connection failed: "));
      Serial.println(influxDBClient.getLastErrorMessage());
    }
  }
}

bool errorInfluxDB() {
  return (influxDBClient.getServerUrl().length() == 0) || (influxDBClient.getLastErrorMessage().length() != 0);
}

String errorInfluxDBMsg() {
  return influxDBClient.getLastErrorMessage();
}

void writeInfluxDB( float temp, float hum, const float lat, const float lon) {
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
