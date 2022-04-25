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
    virtual ~Settings() {};
    virtual int save(JsonObject& root) = 0;
    virtual int load(JsonObject& root) = 0;
    virtual void print(const __FlashStringHelper *title) = 0;
    virtual String getFilePath() = 0;
    void setHandler(UpdateNotificationHandler handler) { _handler = handler; }
    void notify() { if(_handler) _handler(); }
};

class FSPersistence;

typedef std::function<void(Settings *pSettings, JsonObject JsonObject)> DataManipulator;

class SettingsEndpoint {
public:
    SettingsEndpoint(AsyncWebServer* pServer, const  String &endpointPath, FSPersistence *pPersistence,
        Settings *pSettings, DataManipulator fetchManipulator = nullptr, DataManipulator updateManipulator = nullptr, bool persist = true);
    virtual ~SettingsEndpoint() {};
protected:
    virtual void fetchSettings(AsyncWebServerRequest* request);
    virtual void updateSettings(AsyncWebServerRequest* request, JsonVariant& json);
protected:
    Settings *_pSettings;
    FSPersistence *_pPersistence;
    DataManipulator _fetchManipulator;
    DataManipulator _updateManipulator;
    bool _persist;
};

extern const char *ReplaceMark;
// Obfuscates at least 4 letters long token by placing ReplaceMark after first 4 chars and adds last 4 chars of the token
String obfuscateToken(const String &token, int cut = 4);

#endif //SETTINGS_H
