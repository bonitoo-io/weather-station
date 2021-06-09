#ifndef WEATHER_STATION_H
#define WEATHER_STATION_H

#include <Arduino.h>

#include <ESPAsyncWebServer.h>


#include "WiFi.h"
#include "FSPersistance.h"
#include "System.h"
#include "InfluxDB.h"


class WeatherStation {
public:
    WeatherStation(AsyncWebServer* server);

    void begin();
    void loop();

private:
    WiFiSettings _wifiSettings;
    InfluxDBSettings _influxDBSettings;
    FSPersistence _persistence;
    WiFiStationService _wifiStationService;
    WifiAccessPointService _wifiAPService;
    WiFiScannerEndpoint _wifiScanner;
    SettingsEndpoint _wifiSettingsEndpoint;
    SettingsEndpoint _influxDBSettingsEndpoint;
    WiFiStatusEndpoint _wifiStatusEndpoint;
    SystemStatusEndpoint _systemStatusEndpoint;
    SystemServiceEndpoint _systemServiceEndpoint;
};

#endif //WEATHER_STATION_H
