#ifndef SYSTEM_H
#define SYSTEM_H

#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>


#define SYSTEM_STATUS_ENDPOINT_PATH "/api/systemStatus"

class SystemStatusEndpoint {
 public:
  SystemStatusEndpoint(AsyncWebServer* server, FS* fs);

 private:
  FS *_fs;
  void systemStatusHandler(AsyncWebServerRequest* request);
};

#define SYSTEM_RESTART_ENDPOINT_PATH "/api/restart"
#define SYSTEM_FACTORY_RESET_ENDPOINT_PATH "/api/factoryReset"

class SystemServiceEndpoint {
 public:
  SystemServiceEndpoint(AsyncWebServer* server, FS* fs);
  void factoryReset();

  static void restartNow() {
    WiFi.disconnect(true);
    delay(500);
    ESP.restart();
  }

 private:
  void restartHandler(AsyncWebServerRequest* request);
  void factoryResetHandler(AsyncWebServerRequest* request);
 private:
  FS *_fs;
};

#endif  // end SystemStatus_h
