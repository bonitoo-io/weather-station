#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "WiFi.h"
#include "FSPersistance.h"
#include "System.h"
#include "InfluxDBHelper.h"


class WeatherStation {
public:
    WeatherStation();
    void begin();
    void loop();

    WiFiSettings *getWifiSettings() {
        return &_wifiSettings;
    }

    InfluxDBSettings *getInfluxDBSettings() {
        return &_influxDBSettings;
    }
    FSPersistence *getPersistence() {
        return &_persistence;
    }
    WiFiManager *getWifiManager() {
        return &_wifiManager;
    }
private:
    AsyncWebServer _server;
    WiFiSettings _wifiSettings;
    InfluxDBSettings _influxDBSettings;
    FSPersistence _persistence;
    WiFiManager _wifiManager;
    WiFiScannerEndpoint _wifiScanner;
    SettingsEndpoint _wifiSettingsEndpoint;
    SettingsEndpoint _influxDBSettingsEndpoint;
    WiFiStatusEndpoint _wifiStatusEndpoint;
    SystemStatusEndpoint _systemStatusEndpoint;
    SystemServiceEndpoint _systemServiceEndpoint;
};
#endif //WEATHER_STATION_H
