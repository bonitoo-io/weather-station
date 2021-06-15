#include "Settings.h"
#include "IPUtils.h"
#include "FSPersistance.h"

// ****************** WiFiSettingEndpoint ***************************

SettingsEndpoint::SettingsEndpoint(AsyncWebServer* server, const char *endpointPath, FSPersistence *persistence, Settings *settings):
    _settings(settings), _persistence(persistence),
    _updateHandler(endpointPath, 
                     std::bind(&SettingsEndpoint::updateSettings, this, std::placeholders::_1, std::placeholders::_2),
                     DEFAULT_BUFFER_SIZE) {
    _updateHandler.setMethod(HTTP_POST);
    server->addHandler(&_updateHandler);
    server->on(endpointPath, HTTP_GET, std::bind(&SettingsEndpoint::fetchSettings, this, std::placeholders::_1));
}

void SettingsEndpoint::fetchSettings(AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    JsonObject jsonObject = response->getRoot().to<JsonObject>();
    _settings->save(jsonObject);

    response->setLength();
    request->send(response);
}

void SettingsEndpoint::updateSettings(AsyncWebServerRequest* request, JsonVariant& json) {
    if (!json.is<JsonObject>()) {
        request->send(400);
        return;
    }
    JsonObject jsonObject = json.as<JsonObject>();
    //TODO: double JSON serialization
    _settings->load(jsonObject);
    _persistence->writeToFS(_settings);
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    jsonObject = response->getRoot().to<JsonObject>();
    _settings->save(jsonObject);
    request->onDisconnect([this]() { _settings->notify(); });
    response->setLength();
    request->send(response);
}
