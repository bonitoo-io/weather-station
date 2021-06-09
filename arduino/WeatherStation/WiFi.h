#ifndef WS_WIFI_H
#define WS_WIFI_H

#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "Settings.h"
#include "FSPersistance.h"

#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""

#define WIFI_HOSTNAME_PREFIX "weather-station-"
#define WIFI_RECONNECTION_DELAY 1000 * 30
#define WIFI_SETTINGS_ENDPOINT_PATH "/api/wifiSettings"

#define AP_SSID_PREFIX "weather-station-"
#define AP_PASSWORD "influxdata"
#define AP_LOCAL_IP "192.168.4.1"
#define AP_GATEWAY_IP "192.168.4.1"
#define AP_SUBNET_MASK "255.255.255.0"
#define AP_CHANNEL 1
#define AP_SSID_HIDDEN false
#define AP_MAX_CLIENTS 4
#define DNS_PORT 53
#define MANAGE_NETWORK_DELAY 1000*10

class WiFiSettings : public Settings {
public:
    // main configuration
    String ssid;
    String password;
    String hostname;
    bool staticIPConfig;
    // optional configuration for static IP
    IPAddress localIP;
    IPAddress gatewayIP;
    IPAddress subnetMask;
    IPAddress dnsIP1;
    IPAddress dnsIP2;
public:
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual String filePath() override { return F(FS_CONFIG_DIRECTORY "/wifiSettings.json"); }
};

class WiFiStationService {
public:
    WiFiStationService(WiFiSettings *_settings);

    void begin();
    void loop();

private:
    WiFiSettings *_settings;
    unsigned long _lastConnectionAttempt;
    WiFiEventHandler _onStationModeDisconnectedHandler;
    void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);


    void reconfigureWiFiConnection();
    void manageSTA();
};

class WifiAccessPointService  {
public:
    WifiAccessPointService();

    void begin();
    void loop();

 private:
    // for the captive portal
    DNSServer *_dnsServer;

    // for the management delay loop
    unsigned long _lastManaged;
    boolean _reconfigureAp;

    void reconfigureAP();
    void manageAP();
    void startAP();
    void stopAP();
    void handleDNS();
};


#define SCAN_NETWORKS_ENDPOINT_PATH "/api/scanNetworks"
#define LIST_NETWORKS_ENDPOINT_PATH "/api/listNetworks"

class WiFiScannerEndpoint {
public:
    WiFiScannerEndpoint(AsyncWebServer* server);

private:
    void scanNetworks(AsyncWebServerRequest* request);
    void listNetworks(AsyncWebServerRequest* request);

    uint8_t convertEncryptionType(uint8_t encryptionType);
};

#define WIFI_SETTINGS_ENDPOINT_PATH "/api/wifiSettings"

class WiFiSettingsEndpoint {
public:
    WiFiSettingsEndpoint(AsyncWebServer* server, FSPersistence *persistence, WiFiSettings *_settings);
private:
    void fetchSettings(AsyncWebServerRequest* request);
    void updateSettings(AsyncWebServerRequest* request, JsonVariant& json);
private:
    WiFiSettings *_settings;
    FSPersistence *_persistence;
    AsyncCallbackJsonWebHandler _updateHandler;
};

#define WIFI_STATUS_ENDPOINT_PATH "/api/wifiStatus"

class WiFiStatusEndpoint {
 public:
  WiFiStatusEndpoint(AsyncWebServer* server);

 private:
  void wifiStatusHandler(AsyncWebServerRequest* request);
  // handler refrences for logging important WiFi events over serial
  WiFiEventHandler _onStationModeConnectedHandler;
  WiFiEventHandler _onStationModeDisconnectedHandler;
  WiFiEventHandler _onStationModeGotIPHandler;
  // static functions for logging WiFi events to the UART
  static void onStationModeConnected(const WiFiEventStationModeConnected& event);
  static void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);
  static void onStationModeGotIP(const WiFiEventStationModeGotIP& event);
};
#endif //WS_WIFI_H