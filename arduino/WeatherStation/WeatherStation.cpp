#include <LittleFS.h>
#include "WeatherStation.h"
#include "WWWData.h"
#include "Debug.h"

WeatherStation::WeatherStation():
  _server(80),
  _persistence(&LittleFS),
  _wifiManager(&_wifiSettings), 
  _wifiScanner(&_server),
  _wifiSettingsEndpoint(&_server, WIFI_SETTINGS_ENDPOINT_PATH, &_persistence, &_wifiSettings),
  _influxDBSettingsEndpoint(&_server, INFLUXDB_SETTINGS_ENDPOINT_PATH, &_persistence, &_influxDBSettings),
  _wifiStatusEndpoint(&_server),
  _systemStatusEndpoint(&_server, &LittleFS),
  _systemServiceEndpoint(&_server, &_persistence)
  {
#if 1
  // Serve static resources from PROGMEM
  WWWData::registerRoutes(
    [this](const String& uri, const String& contentType, const uint8_t* content, size_t len) {
      ArRequestHandlerFunction requestHandler = [uri,contentType, content, len](AsyncWebServerRequest* request) {
        Serial.print(F("Serving "));
        Serial.print(uri);
        uint32_t s = millis();
        AsyncWebServerResponse* response = request->beginResponse_P(200, contentType, content, len);
        response->addHeader(F("Content-Encoding"), F("gzip"));
        request->send(response);
        Serial.print(F(" in "));
        Serial.println(millis()-s);
        WS_DEBUG_RAM("  RAM");
      };
      _server.on(uri.c_str(), HTTP_GET, requestHandler);
      // Serving non matching get requests with "/index.html"
      // OPTIONS get a straight up 200 response
      if (uri.equals(F("/index.html"))) {
        _server.onNotFound([requestHandler](AsyncWebServerRequest* request) {
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
#endif
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
  
  _wifiManager.begin();
  _server.begin();
}

void WeatherStation::loop() {
  _wifiManager.loop();
}

void WeatherStation::end() {
  _wifiManager.end();
}
