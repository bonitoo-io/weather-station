#include "About.h"
#include "Settings.h"
#include <AsyncJson.h>
#include "DHTSensor.h"
#include "Version.h"

AboutInfoEndpoint::AboutInfoEndpoint(AsyncWebServer *server, tConfig *conf, InfluxDBHelper *influxDBHelper, InfluxDBSettings *influxDBSettings, WiFiSettings *wifiSettings):
  _conf(conf),
  _influxDBHelper(influxDBHelper),
  _influxDBSettings(influxDBSettings),
  _wifiSettings(wifiSettings) {
  server->on(ABOUT_ENDPOINT_PATH,
             HTTP_GET,
             std::bind(&AboutInfoEndpoint::aboutHandler, this, std::placeholders::_1));
}

void AboutInfoEndpoint::aboutHandler(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  root[F("version")] = VERSION;
  root[F("deviceId")] = getDeviceID();
  root[F("useMetric")] = _conf->useMetric?F("true"):F("false");
  root[F("temp")] = getDHTCachedTemp();
  root[F("hum")] = getDHTCachedHum();
  root[F("uptime")] = millis();
  root[F("freeRam")] = ESP.getFreeHeap();
  AppState state = AppState::Ok;
  if(!_wifiSettings->ssid.length() || !WiFi.isConnected()) {
    state = AppState::WifiConfigNeeded;
  } else if(!_influxDBSettings->serverURL.length()) {
    state = AppState::InfluxDBConfigNeeded;
  } else if(_influxDBHelper->isError()) {
      state = AppState::Error;
      root[F("error")] = _influxDBHelper->errorMsg();
  }
  root[F("appState")] = static_cast<int>(state);

  response->setLength();
  request->send(response);
}

AboutServiceEndpoint::AboutServiceEndpoint(AsyncWebServer* server, FSPersistence* persistence):
  _persistence(persistence) {
  server->on(SYSTEM_RESTART_ENDPOINT_PATH, HTTP_POST, std::bind(&AboutServiceEndpoint::restartHandler, this, std::placeholders::_1));
  server->on(SYSTEM_FACTORY_RESET_ENDPOINT_PATH, HTTP_POST, std::bind(&AboutServiceEndpoint::factoryResetHandler, this, std::placeholders::_1));
}

void AboutServiceEndpoint::restartHandler(AsyncWebServerRequest* request) {
  request->onDisconnect(AboutServiceEndpoint::restartNow);
  request->send(200);
}


void AboutServiceEndpoint::factoryResetHandler(AsyncWebServerRequest* request) {
  request->onDisconnect(std::bind(&AboutServiceEndpoint::factoryReset, this));
  request->send(200);
}

/**
 * factoryReset function assumes that all files are stored flat in the config directory.
 */
void AboutServiceEndpoint::factoryReset() {
  Serial.println(F("Factory reset request"));
  _persistence->removeConfigs();
  restartNow();
}
