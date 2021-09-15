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
    virtual String getFilePath() = 0;
    void setHandler(UpdateNotificationHandler handler) { _handler = handler; }
    void notify() { if(_handler) _handler(); }
};

class FSPersistence;

typedef std::function<void(Settings *pSettings, JsonObject JsonObject)> DataManipulator;

class SettingsEndpoint {
public:
    SettingsEndpoint(AsyncWebServer* pServer, const char *endpointPath, FSPersistence *pPersistence, 
        Settings *pSettings, DataManipulator fetchManipulator = nullptr, DataManipulator updateManipulator = nullptr);
protected:
    void fetchSettings(AsyncWebServerRequest* request);
    void updateSettings(AsyncWebServerRequest* request, JsonVariant& json);
protected:
    Settings *_pSettings;
    FSPersistence *_pPersistence;
    DataManipulator _fetchManipulator;
    DataManipulator _updateManipulator;
};

#endif //SETTINGS_H
