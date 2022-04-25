#ifndef WS_WIFI_SETTINGS_H
#define WS_WIFI_SETTINGS_H

#include <ESP8266WiFi.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include "Settings.h"

#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""

#define WIFI_CONFIG_DIRECTORY FS_CONFIG_DIRECTORY "/wifi"
#define WIFI_HOSTNAME_PREFIX "weather-station-"
#define WIFI_SETTINGS_ENDPOINT_PATH "/api/wifiSettings"
#define WIFI_SETTINGS_OLD_FILE_NAME "wifiSettings.json"

extern const char *StringSSID;

class WiFiSettings : public Settings {
public:
    // main configuration
    String ssid;
    String password;
    String hostname;
    bool staticIPConfig;
    // optional configuration for static IP
    IPAddress localIP;
    IPAddress gatewayIP;
    IPAddress subnetMask;
    IPAddress dnsIP1;
    IPAddress dnsIP2;
private:
    String _filePath;
public:
    WiFiSettings();
    void setFilePath(const String &filePath) { _filePath = filePath; }
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual void print(const __FlashStringHelper *title) override;
    virtual String getFilePath() override { return  _filePath; }
private:
    static char *DefaultHostname;
};

class WiFiSettingsEndpoint : public SettingsEndpoint {
public:
    WiFiSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, Settings *pSettings);
};

#endif //WS_WIFI_SETTINGS_H
