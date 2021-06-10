#include "WeatherStation.h"
#include "WWWData.h"

WeatherStation::WeatherStation(AsyncWebServer* server):
    _persistence(&LittleFS),
    _wifiStationService(&_wifiSettings), 
    _wifiScanner(server),
    _wifiSettingsEndpoint(server, WIFI_SETTINGS_ENDPOINT_PATH, &_persistence, &_wifiSettings),
    _influxDBSettingsEndpoint(server, INFLUXDB_SETTINGS_ENDPOINT_PATH, &_persistence, &_influxDBSettings),
    _wifiStatusEndpoint(server),
    _systemStatusEndpoint(server, &LittleFS),
    _systemServiceEndpoint(server, &LittleFS) {

    // Serve static resources from PROGMEM
    WWWData::registerRoutes(
      [server, this](const String& uri, const String& contentType, const uint8_t* content, size_t len) {
        ArRequestHandlerFunction requestHandler = [uri,contentType, content, len](AsyncWebServerRequest* request) {
          Serial.print(F("Serving "));
          Serial.print(uri);
          uint32_t s = millis();
          AsyncWebServerResponse* response = request->beginResponse_P(200, contentType, content, len);
          response->addHeader(F("Content-Encoding"), F("gzip"));
          request->send(response);
          Serial.print(F(" in "));
          Serial.println(millis()-s);
           Serial.printf_P(PSTR("RAM 4: %d\n"), ESP.getFreeHeap());
        };
        server->on(uri.c_str(), HTTP_GET, requestHandler);
        // Serving non matching get requests with "/index.html"
        // OPTIONS get a straight up 200 response
        if (uri.equals(F("/index.html"))) {
          server->onNotFound([requestHandler](AsyncWebServerRequest* request) {
            if (request->method() == HTTP_GET) {
              requestHandler(request);
            } else if (request->method() == HTTP_OPTIONS) {
              request->send(200);
            } else {
              request->send(404);
            }
          });
        }
      });

// Enable CORS for UI development
#if 1
    DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("http://localhost:3000"));
    DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("Accept, Content-Type, Authorization"));
    DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Credentials"), F("true"));
#endif
}

void WeatherStation::begin() {
    // Must be called first before accessing FS
    _persistence.begin();
    _persistence.readFromFS(&_wifiSettings);
    _persistence.readFromFS(&_influxDBSettings);

    _wifiStationService.begin();
    _wifiAPService.begin();
}



void WeatherStation::loop() {
    _wifiStationService.loop();
    _wifiAPService.loop();
}

