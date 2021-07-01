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
private:
    AsyncWebServer _server;
    WiFiSettings _wifiSettings;
    InfluxDBSettings _influxDBSettings;
    UpdaterSettings _updaterSettings;
    FSPersistence _persistence;
    WiFiManager _wifiManager;
    WiFiScannerEndpoint _wifiScanner;
    SettingsEndpoint _wifiSettingsEndpoint;
    InfluxDBSettingsEndpoint _influxDBSettingsEndpoint;
    SettingsEndpoint _updaterSettingsEndpoint;
    WiFiStatusEndpoint _wifiStatusEndpoint;
    AboutInfoEndpoint _aboutInfoEndpoint;
    AboutServiceEndpoint _aboutServiceEndpoint;
    InfluxDBValidateParamsEndpoint _influxdbValidateEndpoint;
};
#endif //WEATHER_STATION_H
