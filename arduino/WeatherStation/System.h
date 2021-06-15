#ifndef SYSTEM_H
#define SYSTEM_H
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include "FSPersistance.h"

#define SYSTEM_STATUS_ENDPOINT_PATH "/api/systemStatus"

class SystemStatusEndpoint {
 public:
  SystemStatusEndpoint(AsyncWebServer *server, FS *fs);

 private:
  FS *_fs;
  void systemStatusHandler(AsyncWebServerRequest* request);
};

#define SYSTEM_RESTART_ENDPOINT_PATH "/api/restart"
#define SYSTEM_FACTORY_RESET_ENDPOINT_PATH "/api/factoryReset"

class SystemServiceEndpoint {
 public:
  SystemServiceEndpoint(AsyncWebServer* server, FSPersistence* persistence);
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
