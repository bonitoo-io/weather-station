#include <LittleFS.h>
#include "WeatherStation.h"
#include "WWWData.h"
#include "Debug.h"
#include "Migrator.h"

WeatherStation::WeatherStation(tConfig *conf, InfluxDBHelper *influxDBHelper):
  _conf(conf),
  _influxDBHelper(influxDBHelper),
  _persistence(&LittleFS),
  _wifiManager(&_persistence,&_wifiSettings) 
  {
// Enable CORS for UI development
#if 1
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("http://localhost:3000"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("Accept, Content-Type, Authorization"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("POST, GET, OPTIONS, DELETE"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Credentials"), F("true"));
#endif
}

void WeatherStation::begin() {
  // Must be called first before accessing FS
  _persistence.begin();
  Serial.println(F("Existing configs:"));
  _persistence.traverseConfigs([](const String &path, const String &filename){
    Serial.print(F("  "));
    Serial.print(path);
    Serial.print(F("/"));
    Serial.println(filename);
  },F("/"));
  Serial.println();

  auto m = new Migrator(&_persistence);
  if(!m->run()) {
    // TODO: set to some global state
    Serial.print(F("Migration error: "));
    Serial.println(m->getError());
  }
  delete m;
  
  //_persistence.readFromFS(&_wifiSettings);
  _persistence.readFromFS(&_influxDBSettings);
  _persistence.readFromFS(&_updaterSettings);
  
  _wifiManager.begin();
}

void WeatherStation::loop() {
  _wifiManager.loop();
  if(_influxdbValidateEndpoint) {
    _influxdbValidateEndpoint->loop();
  }
}

void WeatherStation::end() {
  _wifiManager.end();
}

 void WeatherStation::startServer() {
  if(!_server) {
    Serial.println(F("Starting HTTP server."));
    _server = new AsyncWebServer(80);
    _wifiScannerEndpoint = new WiFiScannerEndpoint(_server);
    _wifiSettingsEndpoint = new WiFiSettingsEndpoint(_server, &_persistence, &_wifiSettings);
    _influxDBSettingsEndpoint = new InfluxDBSettingsEndpoint(_server, &_persistence, &_influxDBSettings);
    _updaterSettingsEndpoint = new SettingsEndpoint(_server, UPDATER_SETTINGS_ENDPOINT_PATH, &_persistence, &_updaterSettings);
    _wifiStatusEndpoint = new WiFiStatusEndpoint(_server, &_wifiManager);
    _aboutInfoEndpoint = new AboutInfoEndpoint(_server, _conf, _influxDBHelper, &_influxDBSettings, &_wifiSettings, &LittleFS);
    _aboutServiceEndpoint = new AboutServiceEndpoint(_server, &_persistence);
    _influxdbValidateEndpoint = new InfluxDBValidateParamsEndpoint(_server, _influxDBHelper);
    _wiFiListSavedEndpoint = new WiFiListSavedEndpoint(_server, &_persistence);
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
        _server->on(uri.c_str(), HTTP_GET, requestHandler);
        // Serving non matching get requests with "/index.html"
        // OPTIONS get a straight up 200 response
        if (uri.equals(F("/index.html"))) {
          _server->onNotFound([requestHandler](AsyncWebServerRequest* request) {
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
    _server->begin();
   }
 }

 void WeatherStation::stopServer() {
   if(_server) {
     Serial.println(F("Stopping HTTP server."));
     // end() and reset() is called in the ~AsyncWebServer
     delete _server;
    _server = nullptr;
    delete _wifiScannerEndpoint;
    _wifiScannerEndpoint = nullptr;
    delete _wifiSettingsEndpoint;
    _wifiSettingsEndpoint = nullptr;
    delete _influxDBSettingsEndpoint;
    _influxDBSettingsEndpoint = nullptr;
    delete _updaterSettingsEndpoint ;
    _updaterSettingsEndpoint = nullptr;
    delete _wifiStatusEndpoint;
    _wifiStatusEndpoint = nullptr;
    delete _aboutInfoEndpoint;
    _aboutInfoEndpoint = nullptr;
    delete _aboutServiceEndpoint;
    _aboutServiceEndpoint = nullptr;
    delete _influxdbValidateEndpoint;
    _influxdbValidateEndpoint = nullptr;
    delete _wiFiListSavedEndpoint;
    _wiFiListSavedEndpoint = nullptr;
   }
 }
