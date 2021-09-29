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

class WeatherStation {
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

    UpdaterSettings *getUpdaterSettings() {
        return &_updaterSettings;
    }

    RegionalSettings *getRegionalSettings() {
        return &_regionalSettings;
    }

    FSPersistence *getPersistence() {
        return &_persistence;
    }

    WiFiManager *getWifiManager() {
        return &_wifiManager;
    }
    void startServer();
    void stopServer();
    void saveRegionalSettings();
    void setFWUploadFinishedCallback(FWUploadFinishedCallback callback) { _fwUploadFinishedCallback = callback; }
private:
    InfluxDBHelper *_influxDBHelper;
    WiFiSettings _wifiSettings;
    InfluxDBSettings _influxDBSettings;
    UpdaterSettings _updaterSettings;
    RegionalSettings _regionalSettings;
    FSPersistence _persistence;
    WiFiManager _wifiManager;
    AsyncWebServer *_server = nullptr;
    WiFiScannerEndpoint *_wifiScannerEndpoint = nullptr;
    WiFiSettingsEndpoint *_wifiSettingsEndpoint = nullptr;
    WiFiConnectionHelperEndpoint *_wifiConnectionHelperEndpoint = nullptr;
    InfluxDBSettingsEndpoint *_influxDBSettingsEndpoint = nullptr;
    UpdaterSettingEnpoint *_updaterSettingsEndpoint = nullptr;
    WiFiStatusEndpoint *_wifiStatusEndpoint = nullptr;
    WiFiListSavedEndpoint *_wiFiListSavedEndpoint = nullptr;
    AboutInfoEndpoint *_aboutInfoEndpoint = nullptr;
    AboutServiceEndpoint *_aboutServiceEndpoint = nullptr;
    InfluxDBValidateParamsEndpoint *_influxdbValidateEndpoint = nullptr;
    SettingsEndpoint *_pRegionalSettingsEndpoint = nullptr;
    RegionalSettingsValidateEndpoint *_pRegionalSettingsValidateEndpoint = nullptr;
    UploadFirmwareEndpoint *_pUploadFirmwareEndpoint = nullptr; 
    FWUploadFinishedCallback _fwUploadFinishedCallback = nullptr;
};

extern WeatherStation station;

#endif //WEATHER_STATION_H
