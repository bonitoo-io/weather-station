#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <functional>

#define DEFAULT_BUFFER_SIZE 1024
#define FS_CONFIG_DIRECTORY "/config"

typedef std::function<void(void)> UpdateNotificationHandler;

class Settings {
private:
    UpdateNotificationHandler _handler;
public:
    virtual int save(JsonObject& root) = 0;
    virtual int load(JsonObject& root) = 0;
    virtual String filePath() = 0;
    void setHandler(UpdateNotificationHandler handler) { _handler = handler; }
    void notify() { if(_handler) _handler(); }
};

class FSPersistence;

class SettingsEndpoint {
public:
    SettingsEndpoint(AsyncWebServer* server, const char *endpointPath, FSPersistence *persistence, Settings *settings);
private:
    void fetchSettings(AsyncWebServerRequest* request);
    void updateSettings(AsyncWebServerRequest* request, JsonVariant& json);
private:
    Settings *_settings;
    FSPersistence *_persistence;
};

#endif //SETTINGS_H
