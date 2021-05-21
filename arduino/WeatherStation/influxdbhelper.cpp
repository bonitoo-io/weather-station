#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

Point sensor("environment"); // Data point
InfluxDBClient influxDBClient;

void setupInfluxDB( const char *serverUrl, const char *org, const char *bucket, const char *authToken) {
  influxDBClient.setConnectionParams(serverUrl, org, bucket, authToken, InfluxDbCloud2CACert);
}


void updateInfluxDB( bool firstStart, const String deviceID, const String location) {
  // Check server connection
  if (firstStart) {
    sensor.addTag("clientId", deviceID);
    sensor.addTag("Device", "WS-ESP8266");
    sensor.addTag("Location", location);
    sensor.addTag("TemperatureSensor", "DHT11");
    sensor.addTag("HumiditySensor", "DHT11");

    if (influxDBClient.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(influxDBClient.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(influxDBClient.getLastErrorMessage());
    }
  }
}

bool errorInfluxDB() {
  return (influxDBClient.getServerUrl() == "") || (influxDBClient.getLastErrorMessage() != "");
}

String errorInfluxDBMsg() {
  return influxDBClient.getLastErrorMessage();
}

void writeInfluxDB( float temp, float hum, const float lat, const float lon) {
  if (influxDBClient.getLastErrorMessage() != "")
    influxDBClient.validateConnection();

  sensor.clearFields();
  // Report temperature and humidity
  sensor.addField("Temperature", temp);
  sensor.addField("Humidity", hum);
  sensor.addField("Lat", lat, 6);
  sensor.addField("Lon", lon, 6);

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(influxDBClient.pointToLineProtocol(sensor));

  // Write point
  if (!influxDBClient.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(influxDBClient.getLastErrorMessage());
  }
}
