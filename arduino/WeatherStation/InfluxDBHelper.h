
#ifndef INFLUXDB_HELPER_H
#define INFLUXDB_HELPER_H


#include "Settings.h"


#define INFLUXDB_DEFAULT_BUCKET F("iot_center")
#define INFLUXDB_DEFAULT_WRITE_INTERVAL 1
#define INFLUXDB_SETTINGS_ENDPOINT_PATH "/api/influxDbSettings" //Cannot be FlashString

class InfluxDBSettings : public Settings {
 public:
  String serverURL;
  String authorizationToken;
  String org;
  String bucket;
  unsigned int writeInterval;
 public:
  InfluxDBSettings();
  virtual int save(JsonObject& root) override;
  virtual int load(JsonObject& root) override;
  virtual String filePath() override { return F(FS_CONFIG_DIRECTORY "/influxDbSettings.json"); }  
};

bool errorInfluxDB();
String errorInfluxDBMsg();
void setupInfluxDB( InfluxDBSettings *settings);
void updateInfluxDB( bool firstStart, const String &deviceID, const String &bucket, const String &wifi, const String &version, const String &location, bool metric);
void writeInfluxDB( float temp, float hum, const float lat, const float lon);

#endif //INFLUXDB_HELPER_H
