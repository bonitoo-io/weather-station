
#ifndef INFLUXDB_HELPER_H
#define INFLUXDB_HELPER_H


#include "Settings.h"
#include <InfluxDbClient.h>
#include "Validation.h"

#define INFLUXDB_DEFAULT_SERVER_URL ""
#define INFLUXDB_DEFAULT_ORG ""
#define INFLUXDB_DEFAULT_BUCKET F("iot_center")
#define INFLUXDB_DEFAULT_TOKEN ""
#define INFLUXDB_DEFAULT_WRITE_INTERVAL 1
#define INFLUXDB_SETTINGS_ENDPOINT_PATH "/api/influxDbSettings" //Cannot be FlashString

#include "custom_dev.h"

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
  virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/influxDbSettings.json"); }  
};


class InfluxDBHelper {
 public:
  InfluxDBHelper();
  bool isError() {
    return _client && (!_client->getServerUrl().length() || _client->getLastErrorMessage().length());
  }
  String errorMsg() { 
    return _client?_client->getLastErrorMessage():"";
  }
  void begin( InfluxDBSettings *settings);
  void update( bool firstStart, const String &deviceID,  const String &wifi, const String &version, const String &location, bool metric);
  void writeStatus(const String &resetReason);
  void write( float temp, float hum, const float lat, const float lon);
  void loadTempHistory(const String &deviceID, bool metric);
  void release();
  String validateConnection(const String &serverUrl, const String &org, const String &bucket, const String &authToken);
  InfluxDBSettings *settings() { return _settings; }
private:
  InfluxDBSettings *_settings = nullptr;
  InfluxDBClient *_client = nullptr;
  Point _sensor; // Data point
};


class InfluxDBSettingsEndpoint : public SettingsEndpoint {
public:
    InfluxDBSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, InfluxDBSettings *pSettings);
};

#define VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH "/api/validateInfluxDBParams"


class InfluxDBValidateParamsEndpoint : public ValidateParamsEndpoint {
public:
    InfluxDBValidateParamsEndpoint(AsyncWebServer* server, InfluxDBHelper *helper);
    virtual ~InfluxDBValidateParamsEndpoint() { delete _validationSettings; }
protected:
  virtual void saveParams(JsonVariant& json) override;
  virtual void runValidation() override;
private:
    InfluxDBSettings *_validationSettings;
    InfluxDBHelper *_helper;
};

#endif //INFLUXDB_HELPER_H
