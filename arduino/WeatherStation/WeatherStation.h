#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "WiFi.h"
#include "FSPersistance.h"
#include "About.h"
#include "InfluxDBHelper.h"
#include "Updater.h"

class WeatherStation {
public:
    WeatherStation(tConfig *conf, InfluxDBHelper *influxDBHelper);
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

    FSPersistence *getPersistence() {
        return &_persistence;
    }

    WiFiManager *getWifiManager() {
        return &_wifiManager;
    }
    void startServer();
    void stopServer();
private:
    tConfig *_conf; 
    InfluxDBHelper *_influxDBHelper;
    WiFiSettings _wifiSettings;
    InfluxDBSettings _influxDBSettings;
    UpdaterSettings _updaterSettings;
    FSPersistence _persistence;
    WiFiManager _wifiManager;
    AsyncWebServer *_server = nullptr;
    WiFiScannerEndpoint *_wifiScannerEndpoint = nullptr;
    SettingsEndpoint *_wifiSettingsEndpoint = nullptr;
    InfluxDBSettingsEndpoint *_influxDBSettingsEndpoint = nullptr;
    SettingsEndpoint *_updaterSettingsEndpoint = nullptr;
    WiFiStatusEndpoint *_wifiStatusEndpoint = nullptr;
    AboutInfoEndpoint *_aboutInfoEndpoint = nullptr;
    AboutServiceEndpoint *_aboutServiceEndpoint = nullptr;
    InfluxDBValidateParamsEndpoint *_influxdbValidateEndpoint = nullptr;
};
#endif //WEATHER_STATION_H
