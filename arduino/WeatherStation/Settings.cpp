#include "Settings.h"
#include "IPUtils.h"
#include "FSPersistance.h"

SettingsEndpoint::SettingsEndpoint(AsyncWebServer* pServer, const char *endpointPath, FSPersistence *pPersistence, Settings *pSettings, DataManipulator fetchManipulator, DataManipulator updateManipulator):
    _pSettings(pSettings), 
    _pPersistence(pPersistence),
    _fetchManipulator(fetchManipulator),
    _updateManipulator(updateManipulator)
     {
    AsyncCallbackJsonWebHandler *updateHandler = new AsyncCallbackJsonWebHandler(endpointPath, 
                std::bind(&SettingsEndpoint::updateSettings, this, std::placeholders::_1, std::placeholders::_2),
                DEFAULT_BUFFER_SIZE);
    updateHandler->setMethod(HTTP_POST);
    pServer->addHandler(updateHandler);
    pServer->on(endpointPath, HTTP_GET, std::bind(&SettingsEndpoint::fetchSettings, this, std::placeholders::_1));
}

void SettingsEndpoint::fetchSettings(AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    JsonObject jsonObject = response->getRoot().to<JsonObject>();
    _pSettings->save(jsonObject);
    if(_fetchManipulator) {
        _fetchManipulator(_pSettings, jsonObject);
    }
    response->setLength();
    request->send(response);
}

void SettingsEndpoint::updateSettings(AsyncWebServerRequest* request, JsonVariant& json) {
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
    _pPersistence->writeToFS(_pSettings);
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    jsonObject = response->getRoot().to<JsonObject>();
    _pSettings->save(jsonObject);
    request->onDisconnect([this]() { _pSettings->notify(); });
    response->setLength();
    request->send(response);
}
