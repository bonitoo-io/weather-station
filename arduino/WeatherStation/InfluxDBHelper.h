
#ifndef INFLUXDB_HELPER_H
#define INFLUXDB_HELPER_H


#include "Settings.h"
#include <InfluxDbClient.h>

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
  void write( float temp, float hum, const float lat, const float lon);
  void loadTempHistory(const String &deviceID, bool metric);
  static String validateConnection(const String &serverUrl, const String &org, const String &bucket, const String &authToken);
private:
  InfluxDBSettings *_settings = nullptr;
  InfluxDBClient *_client = nullptr;
  Point _sensor; // Data point
};

#define VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH "/api/validateInfluxDBParams"

enum ValidationStatus {
  Idle = 0,
  StartRequest,
  Running,
  Finished
};

class InfluxDBValidateParamsEndpoint {
public:
    InfluxDBValidateParamsEndpoint(AsyncWebServer* server);
    ~InfluxDBValidateParamsEndpoint() {delete _validationSettings; }
    void loop();
private:
    void validateParams(AsyncWebServerRequest* request, JsonVariant& json);
    void checkStatus(AsyncWebServerRequest* request);
private:
    InfluxDBSettings *_validationSettings;
    ValidationStatus _status;
    String _error;
    AsyncCallbackJsonWebHandler _validateHandler;
};

#endif //INFLUXDB_HELPER_H
