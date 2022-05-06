#ifndef SYSTEM_H
#define SYSTEM_H
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include "FSPersistance.h"
#include "Tools.h"
#include "InfluxDBHelper.h"
#include "WiFi.h"
#include "InfluxDBHelper.h"
#include "RegionalSettings.h"
#include <LittleFS.h>

#define ABOUT_ENDPOINT_PATH "/api/aboutInfo"

enum class AppState {
  Ok = 0,
  Error,
  WifiConfigNeeded,
  InfluxDBConfigNeeded
};

class AboutInfoEndpoint : public Endpoint {
 public:
  AboutInfoEndpoint(InfluxDBHelper *influxDBHelper, InfluxDBSettings *influxDBSettings,
    WiFiSettings *wifiSettings, RegionalSettings *pRegionalSettings, FS* fs);
  virtual void registerEndpoints(EndpointRegistrator *pRegistrator) override;
  virtual ~AboutInfoEndpoint() {}
 private:
  InfluxDBHelper *_influxDBHelper;
  InfluxDBSettings *_influxDBSettings;
  WiFiSettings *_wifiSettings;
  RegionalSettings *_pRegionalSettings;
  FS* _fs;
  void aboutHandler(AsyncWebServerRequest* request, route *);
};

#define SYSTEM_RESTART_ENDPOINT_PATH "/api/restart"
#define SYSTEM_FACTORY_RESET_ENDPOINT_PATH "/api/factoryReset"

class AboutServiceEndpoint : public Endpoint {
 public:
  AboutServiceEndpoint(FSPersistence* persistence);
  virtual ~AboutServiceEndpoint() {}
  void factoryReset();
  
  virtual void registerEndpoints(EndpointRegistrator *pRegistrator) override;

  static void restartNow();
 private:
  void restartHandler(AsyncWebServerRequest* request, JsonVariant& json, route *);
  void factoryResetHandler(AsyncWebServerRequest* request, JsonVariant& json, route *);
 private:
  FSPersistence* _persistence;
};

#endif  // end SystemStatus_h
