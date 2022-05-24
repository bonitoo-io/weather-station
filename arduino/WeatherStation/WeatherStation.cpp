#include <LittleFS.h>
#include "WeatherStation.h"
#include "WWWData.h"
#include "Debug.h"
#include "Migrator.h"
#include "Tools.h"

volatile uint8_t wsState = WSState::AppStateNotInitialised;

WeatherStation::WeatherStation(InfluxDBHelper *influxDBHelper):
  _influxDBHelper(influxDBHelper),
  _persistence(&LittleFS),
  _wifiManager(&_persistence,&_wifiSettings),
  _requestsInProgress(0)
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

  _advancedSettings.begin();

  _persistence.readFromFS(&_influxDBSettings);
  _persistence.readFromFS(&_regionalSettings);
  _persistence.readFromFS(&_advancedSettings);
  _persistence.readFromFS(&_displaySettings);

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

static routeMap getRoutes;
static routeMap postRoutes;
static routeMap deleteRoutes;
static route *indexRoute = nullptr;


void cleanMap(routeMap &map) {
  for(const auto& [key, val] : map) {
    delete val;
  }
  map.clear();
}

void cleanRoutes() {
  cleanMap(getRoutes);
  cleanMap(postRoutes);
  cleanMap(deleteRoutes);
}

void WeatherStation::registerStaticHandler(const char *uri, const char *contentType, const uint8_t* content, size_t len) {
  get_route *r = new get_route;
  r->params = new static_params;
  r->params->contentType = contentType;
  r->params->content = content;
  r->params->len = len;
  r->handler = (GetRequestHandler) std::bind(&WeatherStation::respondStatic, this, std::placeholders::_1, std::placeholders::_2);
  if(getRoutes.find(uri) != getRoutes.end()) {
    Serial.printf_P(PSTR("Error: double registered static uri '%s'\n"), uri);
  }
  getRoutes[uri] = r;
  if(!strcmp_P(uri,PSTR("/index.html"))) {
    indexRoute = r;
  }
}

void WeatherStation::registerGetHandler(const char *uri, GetRequestHandler handler) {
  get_route *r = new get_route;
  r->handler = handler;
  if(getRoutes.find(uri) != getRoutes.end()) {
    Serial.printf_P(PSTR("Error: double registered GET uri '%s'\n"), uri);
  }
  getRoutes[uri] = r;
}

void WeatherStation::registerDeleteHandler(const char *uri, GetRequestHandler handler) {
  get_route *r = new get_route;
  r->handler = handler;
  if(deleteRoutes.find(uri) != deleteRoutes.end()) {
    Serial.printf_P(PSTR("Error: double registered DELETE uri '%s'\n"), uri);
  }
  deleteRoutes[uri] = r;
}

void WeatherStation::registerPostHandler(const char *uri, PostRequestHandler handler) {
  post_route *r = new post_route;
  r->handler = handler;
  if(postRoutes.find(uri) != postRoutes.end()) {
    Serial.printf_P(PSTR("  Error: double registered POST uri '%s'\n"), uri);
  }
  postRoutes[uri] = r;
}


route *WeatherStation::findRoute(routeMap &map, AsyncWebServerRequest* request) {
  const char *uri = request->url().c_str();
  auto it =  map.find(uri);
  if(it == map.end()) {
    Serial.printf_P(PSTR("%d on %s not found\n"), request->method(), uri);
    if(indexRoute) {
      respondStatic(request, indexRoute);
    } else {
      request->send(404);
    }
    return nullptr;
  }
  return it->second;
}

void WeatherStation::getRequestHandler(AsyncWebServerRequest* request) {
  get_route *r = static_cast<get_route *>(findRoute(getRoutes, request));
  if(r) {
    r->handler(request, r);
  }
}

void WeatherStation::deleteRequestHandler(AsyncWebServerRequest* request) {
  get_route *r = static_cast<get_route *>(findRoute(deleteRoutes, request));
  if(r) {
    r->handler(request, r);
  }
}

void WeatherStation::postRequestHandler(AsyncWebServerRequest* request, JsonVariant &json) {
  post_route *r = static_cast<post_route *>(findRoute(postRoutes, request));
  if(r) {
    r->handler(request, json, r);
  }
}

void WeatherStation::respondStatic(AsyncWebServerRequest* request, route *r) {
  if(r == indexRoute) {
    Serial.println(F(" Responding with index"));
  }
  uint32_t s = millis();
  request->onDisconnect([s]() {
    Serial.print(F(" in "));
    Serial.print(millis()-s);
    Serial.println(F("ms "));
  });
  if (request->header("If-None-Match").equals(ETag)) {
    // send not modified
    Serial.println(F(" Not modified"));
    request->send(304);
  } else {
    bool cont = true;
    if(ESP.getMaxFreeBlockSize() < 4096) {
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
      AsyncWebServerResponse* response = request->beginResponse_P(200, r->params->contentType, r->params->content, r->params->len);
      response->addHeader(F("Content-Encoding"), F("gzip"));
      response->addHeader(F("Cache-Control"), F("public, max-age=3600, must-revalidate"));
      response->addHeader(F("ETag"), ETag);
      request->send(response);
    }
  }
};

void WeatherStation::notFound (AsyncWebServerRequest* request) {
  Serial.println(F(" Not found"));
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
  } else {
    respondStatic(request, indexRoute);
  }
}

void WeatherStation::registerStatics() {
  _server->on("/*", HTTP_GET, std::bind(&WeatherStation::getRequestHandler, this, std::placeholders::_1));
  _server->on("/*", HTTP_DELETE, std::bind(&WeatherStation::deleteRequestHandler, this, std::placeholders::_1));

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/*",
    std::bind(&WeatherStation::postRequestHandler, this, std::placeholders::_1, std::placeholders::_2), DEFAULT_BUFFER_SIZE);
  handler->setMethod(HTTP_POST);

  _server->addHandler(handler);

  _server->onNotFound(std::bind(&WeatherStation::notFound, this, std::placeholders::_1));
  // Serve static resources from PROGMEM

    WWWData::registerIndex(std::bind(&WeatherStation::registerStaticHandler,
      this,
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4));
    WWWData::registerRoutes(std::bind(&WeatherStation::registerStaticHandler,
      this,
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3,
      std::placeholders::_4));
}

bool WeatherStation::globalFilterHandler(AsyncWebServerRequest *request) {
  ++_requestsInProgress;
  Serial.printf_P(PSTR("Serving %d on %s. %d req. total\n"), request->method(), request->url().c_str(), _requestsInProgress);
  WS_DEBUG_RAM(" RAM Before request");
  if(_influxDBHelper->isWriting() || wsState & WSState::AppStateDownloadingUpdate || wsState & WSState::AppStateUploadingUpdate ) {
    Serial.print(F(" blocking: "));
    if(_influxDBHelper->isWriting()) {
       Serial.println(F(" writing in progress"));
    } else {
      Serial.printf_P(PSTR("app in state: %d\n"), wsState);
    }
    AsyncWebServerResponse *response = request->beginResponse(429);
    response->addHeader("Retry-After","1");
    request->send(response);
    --_requestsInProgress;
    return false;
  }

  return true;
}

void WeatherStation::globalDisconnectHandler(AsyncWebServerRequest *request) {
  WS_DEBUG_RAM(" RAM after request");
  Serial.print(F("Sent "));
  Serial.println(request->url());
  if(_requestsInProgress > 0) { //this handler is sometimes called with empty request
    --_requestsInProgress;
  }
}

void WeatherStation::startServer() {
  if(!_server) {
    Serial.println(F("Starting HTTP server."));
    _requestsInProgress = 0;
    _server = new AsyncWebServer(80);
    auto code = _server->begin();
    if(code) {
      Serial.printf_P(PSTR(" Server start error: %d\n"), code);
      delete _server;
      _server = nullptr;
      return;
    }
    _server->setFilter(std::bind(&WeatherStation::globalFilterHandler,
      this,
      std::placeholders::_1));
    _server->onDisconnect(std::bind(&WeatherStation::globalDisconnectHandler,
      this,
      std::placeholders::_1));
    WS_DEBUG_RAM("Before register endpoints");
    _pUploadFirmwareEndpoint = new UploadFirmwareEndpoint(_server);
    _pUploadFirmwareEndpoint->setCallbacks(_fwUploadStartedCallback, _fwUploadFinishedCallback);
    WS_DEBUG_RAM("Before register static endpoints");
    registerStatics();
    WS_DEBUG_RAM("Before register API endpoints");
    registerEndpoints();
    WS_DEBUG_RAM("After register endpoints");
   }
 }

void WeatherStation::registerEndpoints() {
  _wifiScannerEndpoint = new WiFiScannerEndpoint();
  _wifiScannerEndpoint->registerEndpoints(this);
  _wifiSettingsEndpoint = new WiFiSettingsEndpoint(&_persistence, &_wifiSettings);
  _wifiSettingsEndpoint->registerEndpoints(this);
  _wifiStatusEndpoint = new WiFiStatusEndpoint();
  _wifiStatusEndpoint->registerEndpoints(this);
  _wifiConnectionHelperEndpoint = new WiFiConnectionHelperEndpoint(&_wifiManager);
  _wifiConnectionHelperEndpoint->registerEndpoints(this);
  _wiFiListSavedEndpoint = new WiFiListSavedEndpoint(_wifiManager.getWifiSettingsManager());
  _wiFiListSavedEndpoint->registerEndpoints(this);

  _influxDBSettingsEndpoint = new InfluxDBSettingsEndpoint(&_persistence, &_influxDBSettings);
  _influxDBSettingsEndpoint->registerEndpoints(this);
  _influxdbValidateEndpoint = new InfluxDBValidateParamsEndpoint(_influxDBHelper);
  _influxdbValidateEndpoint->registerEndpoints(this);

  _aboutInfoEndpoint = new AboutInfoEndpoint(_influxDBHelper, &_influxDBSettings, &_wifiSettings, &_regionalSettings, &LittleFS);
  _aboutInfoEndpoint->registerEndpoints(this);
  _aboutServiceEndpoint = new AboutServiceEndpoint(&_persistence);
  _aboutServiceEndpoint->registerEndpoints(this);

  _pRegionalSettingsEndpoint = new RegionalSettingsEndpoint(&_persistence, &_regionalSettings);
  _pRegionalSettingsEndpoint->registerEndpoints(this);
  _pRegionalSettingsValidateEndpoint = new RegionalSettingsValidateEndpoint(&_regionalSettings, &_advancedSettings);
  _pRegionalSettingsValidateEndpoint->registerEndpoints(this);

  _pAdvancedSettingsEndpoint = new AdvancedSettingsEndpoint(&_persistence, &_advancedSettings, &_regionalSettings);
  _pAdvancedSettingsEndpoint->registerEndpoints(this);
  _pAdvancedSettingsValidateEndpoint = new AdvancedSettingsValidateEndpoint(&_regionalSettings);
  _pAdvancedSettingsValidateEndpoint->registerEndpoints(this);

  _pDisplaySettingsEndpoint = new DisplaySettingsEndpoint(&_persistence, &_displaySettings, &_regionalSettings);
  _pDisplaySettingsEndpoint->registerEndpoints(this);
 }

 void WeatherStation::stopServer() {
   if(_server) {
    Serial.println(F("Stopping HTTP server."));
     // end() and reset() is called in the ~AsyncWebServer
    delete _server;
    _server = nullptr;

    delete _pUploadFirmwareEndpoint;
    _pUploadFirmwareEndpoint = nullptr;

    delete _wifiScannerEndpoint;
    _wifiScannerEndpoint = nullptr;
    delete _wifiSettingsEndpoint;
    _wifiSettingsEndpoint = nullptr;
    delete _wifiStatusEndpoint;
    _wifiStatusEndpoint = nullptr;
    delete _wifiConnectionHelperEndpoint;
    _wifiConnectionHelperEndpoint = nullptr;
    delete _wiFiListSavedEndpoint;
    _wiFiListSavedEndpoint = nullptr;

    delete _aboutInfoEndpoint;
    _aboutInfoEndpoint = nullptr;
    delete _aboutServiceEndpoint;
    _aboutServiceEndpoint = nullptr;

    delete _influxDBSettingsEndpoint;
    _influxDBSettingsEndpoint = nullptr;
    delete _influxdbValidateEndpoint;
    _influxdbValidateEndpoint = nullptr;

    delete _pRegionalSettingsEndpoint;
    _pRegionalSettingsEndpoint = nullptr;
    delete _pRegionalSettingsValidateEndpoint;
    _pRegionalSettingsValidateEndpoint = nullptr;

    delete _pAdvancedSettingsEndpoint;
    _pAdvancedSettingsEndpoint = nullptr;
    delete _pAdvancedSettingsValidateEndpoint;
    _pAdvancedSettingsValidateEndpoint = nullptr;

    delete _pDisplaySettingsEndpoint;
    _pDisplaySettingsEndpoint = nullptr;
    cleanRoutes();
   }
 }

 void WeatherStation::saveRegionalSettings() {
   _persistence.writeToFS(&_regionalSettings);
 }
