#include <LittleFS.h>
#include "WeatherStation.h"
#include "WWWData.h"
#include "Debug.h"
#include "Migrator.h"
#include "Tools.h"

WeatherStation::WeatherStation(InfluxDBHelper *influxDBHelper):
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
  
  _persistence.readFromFS(&_influxDBSettings);
  _persistence.readFromFS(&_updaterSettings);
  _persistence.readFromFS(&_regionalSettings);
  _persistence.readFromFS(&_advancedSettings);
  
  _wifiManager.begin();
}

void WeatherStation::loop() {
  _wifiManager.loop();
  if(_influxdbValidateEndpoint) {
    _influxdbValidateEndpoint->loop();
  }
  if(_pRegionalSettingsValidateEndpoint) {
    _pRegionalSettingsValidateEndpoint->loop();
  }
  if(_pUploadFirmwareEndpoint) {
    _pUploadFirmwareEndpoint->loop();
  }
  if(_pAdvancedSettingsValidateEndpoint) {
    _pAdvancedSettingsValidateEndpoint->loop();
  }
}

void WeatherStation::end() {
  _wifiManager.end();
}

void WeatherStation::registerHandler(const String& uri, const String& contentType, const uint8_t* content, size_t len) {
  ArRequestHandlerFunction requestHandler = [uri,contentType, content, len, this](AsyncWebServerRequest* request) {
    if(!_endpointsRegistered) {
      registerEndpoints();
    }
    uint32_t s = millis();
    request->onDisconnect([s]() {
      Serial.print(F(" in "));
      Serial.print(millis()-s);
      Serial.println(F("ms "));
    });  
    if (request->header("If-Modified-Since").equals(htmlBuildTime)) {
      // send not modified
      Serial.println(F(" Not modified"));
      request->send(304);
    } else {
      bool cont = true;
      if(ESP.getMaxFreeBlockSize() < 2048) {
        Serial.println(F(" Low memory condition. Releasing influxdb client "));
        if(!_influxDBHelper->release()) {
            Serial.println(F(" Failed "));
            AsyncWebServerResponse* response = request->beginResponse(429);
            response->addHeader(F("Retry-After"), F("1"));
            request->send(response);
            cont = false;
        }
      }
      if(cont) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, contentType, content, len);
        response->addHeader(F("Content-Encoding"), F("gzip"));
        response->addHeader(F("Last-Modified"), htmlBuildTime);
        request->send(response);
      }
    }
  };
  
  _server->on(uri.c_str(), HTTP_GET, requestHandler);
  // Serving non matching get requests with "/index.html"
  // OPTIONS get a straight up 200 response
  if (uri.equals(F("/index.html"))) {
    _server->onNotFound([this,requestHandler](AsyncWebServerRequest* request) {
      Serial.println(F(" Not found"));
      bool ewr = _endpointsRegistered;
      if(!_endpointsRegistered) {
        registerEndpoints();
      }
      if(!ewr && request->url().startsWith(F("/api/"))) {
        AsyncWebServerResponse *response = request->beginResponse(503);
        response->addHeader(F("Retry-After"),"1");
        request->send(response);
      } else if (request->method() == HTTP_GET) {
        requestHandler(request);
      } else if (request->method() == HTTP_OPTIONS) {
        request->send(200);
      } else {
        requestHandler(request);
      }
    });
  }
}

bool WeatherStation::globalFilterHandler(AsyncWebServerRequest *request) {
  Serial.print(F("Serving "));
  Serial.println(request->url());
  WS_DEBUG_RAM(" RAM Before request");
  if(_influxDBHelper->isWriting()) {
    Serial.println(F(" blocking, writing in progress"));
    AsyncWebServerResponse *response = request->beginResponse(429);
    response->addHeader("Retry-After","1");
    request->send(response);
    return false;
  }
  return true;
}

void WeatherStation::globalDisconnectHandler(AsyncWebServerRequest *request) {
  WS_DEBUG_RAM(" RAM after request");
  Serial.print(F("Sent "));
  Serial.println(request->url());
}

void WeatherStation::startServer() {
  if(!_server) {
    Serial.println(F("Starting HTTP server."));
    _server = new AsyncWebServer(80);
    _server->setFilter(std::bind(&WeatherStation::globalFilterHandler, 
      this, 
      std::placeholders::_1));
    _server->onDisconnect(std::bind(&WeatherStation::globalDisconnectHandler, 
      this, 
      std::placeholders::_1));
    
    // Serve static resources from PROGMEM
    WWWData::registerIndex(std::bind(&WeatherStation::registerHandler, 
      this, 
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4));
    _server->begin();
   }
 }

void WeatherStation::registerEndpoints() {
  if(!_endpointsRegistered) {
    Serial.println(F("Registering endpoints"));
    _endpointsRegistered = true;
    _wifiScannerEndpoint = new WiFiScannerEndpoint(_server);
    _wifiSettingsEndpoint = new WiFiSettingsEndpoint(_server, &_persistence, &_wifiSettings);
    _influxDBSettingsEndpoint = new InfluxDBSettingsEndpoint(_server, &_persistence, &_influxDBSettings);
    _updaterSettingsEndpoint = new UpdaterSettingEnpoint(_server, &_persistence, &_updaterSettings, &_regionalSettings);
    _wifiStatusEndpoint = new WiFiStatusEndpoint(_server);
    _wifiConnectionHelperEndpoint = new WiFiConnectionHelperEndpoint(_server, &_wifiManager);
    _aboutInfoEndpoint = new AboutInfoEndpoint(_server, _influxDBHelper, &_influxDBSettings, &_wifiSettings, &_regionalSettings, &LittleFS);
    _aboutServiceEndpoint = new AboutServiceEndpoint(_server, &_persistence);
    _influxdbValidateEndpoint = new InfluxDBValidateParamsEndpoint(_server, _influxDBHelper);
    _wiFiListSavedEndpoint = new WiFiListSavedEndpoint(_server, &_persistence);
    _pRegionalSettingsEndpoint = new SettingsEndpoint(_server, F(REGIONAL_SETTINGS_ENDPOINT_PATH), &_persistence, &_regionalSettings);
    _pRegionalSettingsValidateEndpoint = new RegionalSettingsValidateEndpoint(_server, &_advancedSettings);
    _pUploadFirmwareEndpoint = new UploadFirmwareEndpoint(_server);
    _pUploadFirmwareEndpoint->setCallback(_fwUploadFinishedCallback);
    _pAdvancedSettingsEndpoint = new AdvancedSettingsEndpoint(_server, &_persistence, &_advancedSettings);
    _pAdvancedSettingsValidateEndpoint = new AdvancedSettingsValidateEndpoint(_server, &_regionalSettings);
    // Serve static resources from PROGMEM
    WWWData::registerRoutes(std::bind(&WeatherStation::registerHandler, 
      this, 
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4));
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
    delete _wifiConnectionHelperEndpoint;
    _wifiConnectionHelperEndpoint = nullptr;
    delete _aboutInfoEndpoint;
    _aboutInfoEndpoint = nullptr;
    delete _aboutServiceEndpoint;
    _aboutServiceEndpoint = nullptr;
    delete _influxdbValidateEndpoint;
    _influxdbValidateEndpoint = nullptr;
    delete _wiFiListSavedEndpoint;
    _wiFiListSavedEndpoint = nullptr;
    delete _pRegionalSettingsEndpoint;
    _pRegionalSettingsEndpoint = nullptr;
    delete _pUploadFirmwareEndpoint;
    _pUploadFirmwareEndpoint = nullptr;
    delete _pAdvancedSettingsEndpoint;
    _pAdvancedSettingsEndpoint = nullptr;
    delete _pAdvancedSettingsValidateEndpoint;
    _pAdvancedSettingsValidateEndpoint = nullptr;
    _endpointsRegistered = false;
   }
 }

 void WeatherStation::saveRegionalSettings() {
   _persistence.writeToFS(&_regionalSettings);
 }
