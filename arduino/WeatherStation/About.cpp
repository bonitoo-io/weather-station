#include "About.h"
#include "Settings.h"
#include <AsyncJson.h>
#include "Sensor.h"
#include "Version.h"

AboutInfoEndpoint::AboutInfoEndpoint(InfluxDBHelper *influxDBHelper, InfluxDBSettings *influxDBSettings,
  WiFiSettings *wifiSettings, RegionalSettings *pRegionalSettings, FS* fs):
  _influxDBHelper(influxDBHelper),
  _influxDBSettings(influxDBSettings),
  _wifiSettings(wifiSettings),
  _pRegionalSettings(pRegionalSettings),
  _fs(fs) {
}


void AboutInfoEndpoint::registerEndpoints(EndpointRegistrator *pRegistrator) {
    pRegistrator->registerGetHandler(ABOUT_ENDPOINT_PATH, std::bind(&AboutInfoEndpoint::aboutHandler, this, std::placeholders::_1, std::placeholders::_2));
}


void AboutInfoEndpoint::aboutHandler(AsyncWebServerRequest* request, route *) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  root[F("version")] = getLongVersion();
  root[F("deviceId")] = getDeviceID();
  root[F("useMetric")] = _pRegionalSettings->useMetricUnits;
  root[F("temp")] = _pRegionalSettings->useMetricUnits ? Sensor::tempF2C(pSensor->getTempF(true)) : pSensor->getTempF(true);
  root[F("hum")] = pSensor->getHum(true);
  root[F("sensorName")] = pSensor->getSensorName();
  root[F("uptime")] = millis();
  AppState state = AppState::Ok;
  if(!_wifiSettings->ssid.length() || !WiFi.isConnected()) {
    state = AppState::WifiConfigNeeded;
  } else if(!_influxDBSettings->serverURL.length()) {
    state = AppState::InfluxDBConfigNeeded;
  } else if(_influxDBHelper->isError()) {
      state = AppState::Error;
      String error = F("InfluxDB error: ");
      error += _influxDBHelper->errorMsg();
      root[F("error")] = error;
  }
  root[F("appState")] = static_cast<int>(state);
  root[F("freeHeap")] = ESP.getFreeHeap();
  root[F("espPlatform")] = F("esp8266");
  root[F("maxAllocHeap")] = ESP.getMaxFreeBlockSize();
  root[F("heapFragmentation")] = ESP.getHeapFragmentation();
  root[F("cpuFreq")] = ESP.getCpuFreqMHz();
  root[F("sketchSize")] = ESP.getSketchSize();
  root[F("freeSketchSpace")] = ESP.getFreeSketchSpace();
  root[F("sdkVersion")] = ESP.getSdkVersion();
  root[F("flashChipSize")] = ESP.getFlashChipSize();
  root[F("flashChipSpeed")] = ESP.getFlashChipSpeed();
  FSInfo fs_info;
  _fs->info(fs_info);
  root[F("fsTotal")] = fs_info.totalBytes;
  root[F("fsUsed")] = fs_info.usedBytes;

  response->addHeader(F("Cache-Control"),F("No-Store"));
  response->setLength();
  request->send(response);
}

AboutServiceEndpoint::AboutServiceEndpoint(FSPersistence* persistence):
  _persistence(persistence) {
}

void AboutServiceEndpoint::registerEndpoints(EndpointRegistrator *pRegistrator) {
    pRegistrator->registerPostHandler(SYSTEM_RESTART_ENDPOINT_PATH, std::bind(&AboutServiceEndpoint::restartHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    pRegistrator->registerPostHandler(SYSTEM_FACTORY_RESET_ENDPOINT_PATH, std::bind(&AboutServiceEndpoint::factoryResetHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}


void AboutServiceEndpoint::restartHandler(AsyncWebServerRequest* request, JsonVariant& , route *) {
  request->onDisconnect(AboutServiceEndpoint::restartNow);
  request->send(200);
}

void AboutServiceEndpoint::restartNow() {
  Serial.println(F("Restart request"));
  WiFi.disconnect(true);
  delay(500);
  ESP.restart();
}


void AboutServiceEndpoint::factoryResetHandler(AsyncWebServerRequest* request, JsonVariant& , route *) {
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
