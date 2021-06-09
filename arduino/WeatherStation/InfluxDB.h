#ifndef INFLUX_DB_H
#define INFLUX_DB_H

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "Settings.h"


#define INFLUXDB_DEFAULT_BUCKET "iot_center"
#define INFLUXDB_DEFAULT_WRITE_INTERVAL 1
#define INFLUXDB_SETTINGS_ENDPOINT_PATH "/api/influxDbSettings"

class InfluxDBSettings : public Settings {
 public:
  String serverURL;
  String authorizationToken;
  String org;
  String bucket;
  unsigned int writeInterval;
 public:
  virtual int save(JsonObject& root) override;
  virtual int load(JsonObject& root) override;
  virtual String filePath() override { return F(FS_CONFIG_DIRECTORY "/influxDbSettings.json"); }  
};


class InfluxDBWriter {
 public:
  InfluxDBWriter();
  bool errorInfluxDB() {
    return (_client.getServerUrl() == "") || (_client.getLastErrorMessage() != "");
  }
  String errorInfluxDBMsg() { 
    return _client.getLastErrorMessage();
  }
  void setupInfluxDB( const String &serverUrl, const String &org, const String &bucket, const String &authToken, int refresh_sec);
  void updateInfluxDB( bool firstStart, const String &deviceID, const String &version, const String &location);
  void writeInfluxDB( float temp, float hum, const float lat, const float lon);
private:
  InfluxDBClient _client;
  Point _sensor; // Data point
};


#endif //INFLUX_DB_H