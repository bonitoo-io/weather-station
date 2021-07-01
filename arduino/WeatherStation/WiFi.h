#ifndef WS_WIFI_H
#define WS_WIFI_H

#include <ESP8266WiFi.h>
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
#define AP_PASSWORD ""
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
    WiFiSettings();
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual String filePath() override { return F(FS_CONFIG_DIRECTORY "/wifiSettings.json"); }
private:
    static char *DefaultHostname;
};

struct APInfo {
    IPAddress ipAddress;
    String ssid;
    String password;
    bool running;
};

enum WifiConnectionEvent {
    ConnectingStarted = 0,
    ConnectingSuccess,
    ConnectingFailed
};

typedef std::function<void(APInfo *apInfo)> APEventHandler;
typedef std::function<void(WifiConnectionEvent event, const char *ssid)> WiFiConnectionEventHandler;

class WiFiManager {
public:
    WiFiManager(WiFiSettings *_settings);

    void begin();
    void loop();
    void end();
    void setAPEventHandler(APEventHandler handler) { 
        _apEventHandler = handler;
    }
     void setWiFiConnectionEventHandler(WiFiConnectionEventHandler handler) { 
        _wifiEventHandler = handler;
    }
 private:
  void reconfigureWiFiConnection();
  void manageSTA();
  void manageAP();
  void startAP();
  void stopAP();
  void handleDNS();
  void onStationModeConnected(const WiFiEventStationModeConnected& event);
  void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);
  void onStationModeGotIP(const WiFiEventStationModeGotIP& event);
  void notifyWifiEvent(WifiConnectionEvent event);
  void notifyAPEvent();
 private:
  WiFiSettings *_settings;
  APInfo _apInfo;
  // for the management delay loop
  unsigned long _lastConnectionAttempt;
  // for the captive portal
  DNSServer *_dnsServer;
  APEventHandler _apEventHandler;
  WiFiConnectionEventHandler _wifiEventHandler;
  // WiFi event handlers
  WiFiEventHandler _onStationModeConnectedHandler;
  WiFiEventHandler _onStationModeDisconnectedHandler;
  WiFiEventHandler _onStationModeGotIPHandler;
  bool _forceAPStop;
  uint8_t _connectAttempts;
};

#if 1
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

#define WIFI_STATUS_ENDPOINT_PATH "/api/wifiStatus"

class WiFiStatusEndpoint {
 public:
  WiFiStatusEndpoint(AsyncWebServer* server);

 private:
  void wifiStatusHandler(AsyncWebServerRequest* request);
};
#endif
#endif //WS_WIFI_H
