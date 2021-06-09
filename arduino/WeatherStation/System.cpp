#include "System.h"
#include "Settings.h"
#include <AsyncJson.h>

SystemStatusEndpoint::SystemStatusEndpoint(AsyncWebServer* server, FS* fs):
  _fs(fs) {
  server->on(SYSTEM_STATUS_ENDPOINT_PATH,
             HTTP_GET,
             std::bind(&SystemStatusEndpoint::systemStatusHandler, this, std::placeholders::_1));
}

void SystemStatusEndpoint::systemStatusHandler(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  root["esp_platform"] = "esp8266";
  root["max_alloc_heap"] = ESP.getMaxFreeBlockSize();
  root["heap_fragmentation"] = ESP.getHeapFragmentation();
  root["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
  root["free_heap"] = ESP.getFreeHeap();
  root["sketch_size"] = ESP.getSketchSize();
  root["free_sketch_space"] = ESP.getFreeSketchSpace();
  root["sdk_version"] = ESP.getSdkVersion();
  root["flash_chip_size"] = ESP.getFlashChipSize();
  root["flash_chip_speed"] = ESP.getFlashChipSpeed();

  FSInfo fs_info;
  _fs->info(fs_info);
  root["fs_total"] = fs_info.totalBytes;
  root["fs_used"] = fs_info.usedBytes;

  response->setLength();
  request->send(response);
}

SystemServiceEndpoint::SystemServiceEndpoint(AsyncWebServer* server, FS* fs):
  _fs(fs) {
  server->on(SYSTEM_RESTART_ENDPOINT_PATH, HTTP_POST, std::bind(&SystemServiceEndpoint::restartHandler, this, std::placeholders::_1));
  server->on(SYSTEM_FACTORY_RESET_ENDPOINT_PATH, HTTP_POST, std::bind(&SystemServiceEndpoint::factoryResetHandler, this, std::placeholders::_1));
}

void SystemServiceEndpoint::restartHandler(AsyncWebServerRequest* request) {
  request->onDisconnect(SystemServiceEndpoint::restartNow);
  request->send(200);
}


void SystemServiceEndpoint::factoryResetHandler(AsyncWebServerRequest* request) {
  request->onDisconnect(std::bind(&SystemServiceEndpoint::factoryReset, this));
  request->send(200);
}

/**
 * factoryReset function assumes that all files are stored flat in the config directory.
 */
void SystemServiceEndpoint::factoryReset() {
  Dir configDirectory = _fs->openDir(FS_CONFIG_DIRECTORY);
  while (configDirectory.next()) {
    String path = FS_CONFIG_DIRECTORY;
    path.concat("/");
    path.concat(configDirectory.fileName());
    _fs->remove(path);
  }
  restartNow();
}

