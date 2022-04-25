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

class AboutInfoEndpoint {
 public:
  AboutInfoEndpoint(AsyncWebServer *server, InfluxDBHelper *influxDBHelper, InfluxDBSettings *influxDBSettings,
    WiFiSettings *wifiSettings, RegionalSettings *pRegionalSettings, FS* fs);

 private:
  InfluxDBHelper *_influxDBHelper;
  InfluxDBSettings *_influxDBSettings;
  WiFiSettings *_wifiSettings;
  RegionalSettings *_pRegionalSettings;
  FS* _fs;
  void aboutHandler(AsyncWebServerRequest* request);
};

#define SYSTEM_RESTART_ENDPOINT_PATH "/api/restart"
#define SYSTEM_FACTORY_RESET_ENDPOINT_PATH "/api/factoryReset"

class AboutServiceEndpoint {
 public:
  AboutServiceEndpoint(AsyncWebServer* server, FSPersistence* persistence);
  void factoryReset();

  static void restartNow() {
    Serial.println(F("Restart request"));
    WiFi.disconnect(true);
    delay(500);
    ESP.restart();
  }

 private:
  void restartHandler(AsyncWebServerRequest* request);
  void factoryResetHandler(AsyncWebServerRequest* request);
 private:
  FSPersistence* _persistence;
};

#endif  // end SystemStatus_h
