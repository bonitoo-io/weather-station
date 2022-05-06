#include "Settings.h"
#include "IPUtils.h"
#include "FSPersistance.h"

SettingsEndpoint::SettingsEndpoint(const char *pEndpointPath, FSPersistence *pPersistence, Settings *pSettings,
    DataManipulator fetchManipulator, DataManipulator updateManipulator, bool persist):
    _pEndpointPath(pEndpointPath),
    _pSettings(pSettings),
    _pPersistence(pPersistence),
    _fetchManipulator(fetchManipulator),
    _updateManipulator(updateManipulator),
    _persist(persist)
     {
}

void SettingsEndpoint::registerEndpoints(EndpointRegistrator *pRegistrator) {
    pRegistrator->registerGetHandler(_pEndpointPath, std::bind(&SettingsEndpoint::fetchSettings, this, std::placeholders::_1, std::placeholders::_2));
    pRegistrator->registerPostHandler(_pEndpointPath, std::bind(&SettingsEndpoint::updateSettings, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void SettingsEndpoint::fetchSettings(AsyncWebServerRequest* request, route *) {
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    JsonObject jsonObject = response->getRoot().to<JsonObject>();
    _pSettings->save(jsonObject);
    if(_fetchManipulator) {
        _fetchManipulator(_pSettings, jsonObject);
    }
    response->addHeader(F("Cache-Control"),F("No-Store"));
    response->setLength();
    request->send(response);
}

void SettingsEndpoint::updateSettings(AsyncWebServerRequest* request, JsonVariant& json, route *) {
    if (!json.is<JsonObject>()) {
        request->send(400);
        return;
    }
    JsonObject jsonObject = json.as<JsonObject>();
    if(_updateManipulator) {
        _updateManipulator(_pSettings, jsonObject);
    }
    //TODO: double JSON serialization
    _pSettings->load(jsonObject);
    if(_persist) {
        _pPersistence->writeToFS(_pSettings);
    }
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    jsonObject = response->getRoot().to<JsonObject>();
    _pSettings->save(jsonObject);
    if(_fetchManipulator) {
        _fetchManipulator(_pSettings, jsonObject);
    }
    request->onDisconnect([this]() { _pSettings->notify(); });
    response->addHeader(F("Cache-Control"),F("No-Store"));
    response->setLength();
    request->send(response);
}


const char *ReplaceMark = "******";

String obfuscateToken(const String &token, int cut) {
  String authToken;
  authToken.reserve(strlen(ReplaceMark)+cut*2+1);
  authToken = token.substring(0,cut);
  authToken += ReplaceMark;
  authToken += token.substring(token.length()-cut);
  return authToken;
}
