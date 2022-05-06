#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "WiFi.h"
#include "WiFiSettings.h"
#include "FSPersistance.h"
#include "About.h"
#include "InfluxDBHelper.h"
#include "Updater.h"
#include "RegionalSettings.h"
#include "UploadFirmware.h"
#include "AdvancedSettings.h"
#include "DisplaySettings.h"
#include "Validation.h"
#include "Endpoint.h"

class WeatherStation : public EndpointRegistrator {
public:
    WeatherStation(InfluxDBHelper *influxDBHelper);
    void begin();
    void loop();
    void end();

    WiFiSettings *getWifiSettings() {
        return &_wifiSettings;
    }

    InfluxDBSettings *getInfluxDBSettings() {
        return &_influxDBSettings;
    }

    RegionalSettings *getRegionalSettings() {
        return &_regionalSettings;
    }

    AdvancedSettings *getAdvancedSettings() {
        return &_advancedSettings;
    }

    DisplaySettings *getDisplaySettings() {
        return &_displaySettings;
    }

    FSPersistence *getPersistence() {
        return &_persistence;
    }

    WiFiManager *getWifiManager() {
        return &_wifiManager;
    }
    bool isRequestInProgress() const { return _requestsInProgress > 0; }
    void startServer();
    void stopServer();
    bool isServerStarted() const { return _server != nullptr; }
    void registerEndpoints();
    void saveRegionalSettings();
    void registerStaticHandler(const char *uri, const char *contentType, const uint8_t* content, size_t len);
    void globalDisconnectHandler(AsyncWebServerRequest *request);
    bool globalFilterHandler(AsyncWebServerRequest *request);
    void setFWUploadFinishedCallback(FWUploadFinishedCallback callback) { _fwUploadFinishedCallback = callback; }
public:
    virtual void registerGetHandler(const char *uri, GetRequestHandler handler) override;
    virtual void registerDeleteHandler(const char *uri, GetRequestHandler handler) override;
    virtual void registerPostHandler(const char *uri, PostRequestHandler handler) override;   
private:
    void getRequestHandler(AsyncWebServerRequest* request);
    void deleteRequestHandler(AsyncWebServerRequest* request);
    void postRequestHandler(AsyncWebServerRequest* request, JsonVariant &json);
    void respondStatic(AsyncWebServerRequest* request, route *r);
    void notFound (AsyncWebServerRequest* request);
    void registerStatics() ;
    route *findRoute(routeMap &map, AsyncWebServerRequest* request);
private:
    volatile uint8_t _requestsInProgress;
    InfluxDBHelper *_influxDBHelper;
    WiFiSettings _wifiSettings;
    InfluxDBSettings _influxDBSettings;
    RegionalSettings _regionalSettings;
    AdvancedSettings _advancedSettings;
    DisplaySettings _displaySettings;
    FSPersistence _persistence;
    WiFiManager _wifiManager;
    AsyncWebServer *_server = nullptr;
    WiFiScannerEndpoint *_wifiScannerEndpoint = nullptr;
    WiFiSettingsEndpoint *_wifiSettingsEndpoint = nullptr;
    WiFiConnectionHelperEndpoint *_wifiConnectionHelperEndpoint = nullptr;
    InfluxDBSettingsEndpoint *_influxDBSettingsEndpoint = nullptr;
    WiFiStatusEndpoint *_wifiStatusEndpoint = nullptr;
    WiFiListSavedEndpoint *_wiFiListSavedEndpoint = nullptr;
    AboutInfoEndpoint *_aboutInfoEndpoint = nullptr;
    AboutServiceEndpoint *_aboutServiceEndpoint = nullptr;
    InfluxDBValidateParamsEndpoint *_influxdbValidateEndpoint = nullptr;
    SettingsEndpoint *_pRegionalSettingsEndpoint = nullptr;
    RegionalSettingsValidateEndpoint *_pRegionalSettingsValidateEndpoint = nullptr;
    UploadFirmwareEndpoint *_pUploadFirmwareEndpoint = nullptr;
    FWUploadFinishedCallback _fwUploadFinishedCallback = nullptr;
    AdvancedSettingsEndpoint *_pAdvancedSettingsEndpoint = nullptr;
    AdvancedSettingsValidateEndpoint *_pAdvancedSettingsValidateEndpoint = nullptr;
    DisplaySettingsEndpoint *_pDisplaySettingsEndpoint = nullptr;
};

extern WeatherStation station;

#endif //WEATHER_STATION_H
