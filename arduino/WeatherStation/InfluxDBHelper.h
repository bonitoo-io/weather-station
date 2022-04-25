
#ifndef INFLUXDB_HELPER_H
#define INFLUXDB_HELPER_H


#include "Settings.h"
#include <InfluxDbClient.h>
#include "Validation.h"

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
  virtual void print(const __FlashStringHelper *title) override;
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
  void registerResetInfo(const String &resetReason);
  bool writeResetInfo();
  bool writeStatus();
  bool write( float temp, float hum, const float lat, const float lon);
  bool loadTempHistory(const String &deviceID);
  bool release();
  String validateConnection(const String &serverUrl, const String &org, const String &bucket, const String &authToken);
  InfluxDBSettings *settings() { return _settings; }
  bool wasReleased() const { return _wasReleased; }
  bool isWriting() const { return _lock == 1; }
  uint16_t getWriteSuccess() { return writeSuccess;}
  void clearWriteSuccess() { writeSuccess = 0;}
private:
  InfluxDBSettings *_settings = nullptr;
  InfluxDBClient *_client = nullptr;
  Point _sensor; // Data point
  Point *_pResetInfo = nullptr;
  bool _wasReleased = false;
  // simple sync mechanism
  volatile uint8_t _lock = 0;
  uint16_t writeSuccess = 0;
};


class InfluxDBSettingsEndpoint : public SettingsEndpoint {
public:
    InfluxDBSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, InfluxDBSettings *pSettings);
};

#define VALIDATE_INFLUXDB_PARAMS_ENDPOINT_PATH "/api/validateInfluxDBParams"


class InfluxDBValidateParamsEndpoint : public ValidateParamsEndpoint {
public:
    InfluxDBValidateParamsEndpoint(AsyncWebServer *pServer, InfluxDBHelper *pHelper);
    virtual ~InfluxDBValidateParamsEndpoint() { delete _validationSettings; }
protected:
  virtual void saveParams(JsonVariant& json) override;
  virtual void runValidation() override;
private:
    InfluxDBSettings *_validationSettings;
    InfluxDBHelper *_pHelper;
};

#endif //INFLUXDB_HELPER_H
