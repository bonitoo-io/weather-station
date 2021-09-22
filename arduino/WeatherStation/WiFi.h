#ifndef WS_WIFI_H
#define WS_WIFI_H

#include <ESP8266WiFi.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "WiFiSettings.h"
#include "FSPersistance.h"


#define WIFI_RECONNECTION_DELAY 1000 * 30

#define AP_SSID_PREFIX "WEATHER-"
#define AP_PASSWORD ""
#define AP_LOCAL_IP "1.1.1.1"
#define AP_GATEWAY_IP "1.1.1.1"
#define AP_SUBNET_MASK "255.255.255.0"
#define AP_CHANNEL 1
#define AP_SSID_HIDDEN false
#define AP_MAX_CLIENTS 4
#define DNS_PORT 53
#define MANAGE_NETWORK_DELAY 1000*30
#define SCAN_NETWORK_DELAY 1000

struct APInfo {
    IPAddress ipAddress;
    String ssid;
    String password;
    bool running;
    uint8_t clientsCount;
};

enum class WifiConnectionEvent {
    ConnectingStarted = 0,
    ConnectingSuccess,
    ConnectingFailed
};

enum class WiFiConnectingState {
    NotConnected = 0,
    ScanStarted,
    ScanFinished,
    ScanFailed,
    ConnectingToKnown, //when conneting using saved list
    ConnectingSuccess,
    ConnectingToHidden,
    TestingConfig,
    TestingConfigFailed,
    ConnectingToSaved, //when conencting from external request
    Idle
};


enum class WifiAPEvent {
    APStarted = 0,
    ClientConnected,
    ClientDisconnected,
    APStopped
};

struct WiFiNetwork
{
  String ssid;
  uint8_t *bssid;
  int32_t rssi;
  int32_t channel;
  bool connectSkip;
};

typedef std::function<void(WifiAPEvent event, APInfo *apInfo)> APEventHandler;
typedef std::function<void(WifiConnectionEvent event, const char *ssid)> WiFiConnectionEventHandler;


class WiFiManager {
public:
    WiFiManager(FSPersistence *pFsp, WiFiSettings *pSettings);

    void begin();
    void loop();
    void end();
    void setAPEventHandler(APEventHandler handler) { 
        _apEventHandler = handler;
    }
     void setWiFiConnectionEventHandler(WiFiConnectionEventHandler handler) { 
        _wifiEventHandler = handler;
    }
    int getLastDisconnectReason() const { return _lastDisconnectReason; }
    bool isConnectingToWiFi() const { return _connectingToWifi; }
    bool isConnectTestSuccessful() const { return _connectTestSuccess; }
    void connectToSavedNetwork(int index);
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
  void onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& event);
  void onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& event);
  void notifyWifiEvent(WifiConnectionEvent event);
  void notifyAPEvent(WifiAPEvent event);
  bool startSTA(WiFiSettings *pConfig, WiFiNetwork *pNetwork = nullptr);
  void enterState(WiFiConnectingState newState, bool reconFigure = false);
  void loadSettings(const String &ssid);
  void cleanNetworks();
  void setPreviousNetwork(const char *network);
 private:
  FSPersistence *_pFsp;
  WiFiSettings *_pSettings;
  APInfo _apInfo;
  WifiAPEvent *_asyncEventToFire = nullptr;
  // Variable manage delay
  uint _manageDelay = MANAGE_NETWORK_DELAY;
  // for the management delay loop
  unsigned long _lastConnectionAttempt = 0;
  // for the captive portal
  DNSServer *_dnsServer = nullptr;
  APEventHandler _apEventHandler = nullptr;
  WiFiConnectionEventHandler _wifiEventHandler = nullptr;
  uint8_t _lastDisconnectReason = 0;
  // WiFi event handlers
  WiFiEventHandler _onStationModeConnectedHandler;
  WiFiEventHandler _onStationModeDisconnectedHandler;
  WiFiEventHandler _onStationModeGotIPHandler;
  WiFiEventHandler _onSoftAPModeStationConnectedHandler;
  WiFiEventHandler _onSoftAPModeStationDisconnectedHandler;
  // time when ap should stop
  uint32_t _forceAPStop = 0;
  uint8_t _connectAttempts = 0;
  // Index to network list when searching for connetable network
  uint8_t _wifiNetworkIndex = 0;
  // list of save networks
  std::vector<String> _savedNetworks;
  // list of currently found networks
  std::vector<WiFiNetwork> _foundNetworks;
  WiFiConnectingState _state = WiFiConnectingState::NotConnected;
  bool _connectingToWifi = false;
  bool _ignoreDisconnect = false;
  // last connected network when try new
  char *_previousNetwork = nullptr;
  // Result of external connection attempt (TestingConfig, ConnectToSaved)
  bool _connectTestSuccess = false;
};

#define CONNECT_TO_SAVED_ENDPOINT_PATH "/api/connectSaved"
#define CONNECT_STATUS_ENDPOINT_PATH "/api/connectStatus"

class WiFiConnectionHelperEndpoint {
public:
  WiFiConnectionHelperEndpoint(AsyncWebServer* server, WiFiManager *pWiFiManager);
private:
  void connectToSaved(AsyncWebServerRequest* request, JsonVariant& json);
  void connectingStatus(AsyncWebServerRequest* request);
private:
  WiFiManager *_pWiFiManager;
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


#define WIFI_STATUS_ENDPOINT_PATH "/api/wifiStatus"

class WiFiStatusEndpoint {
  public:
    WiFiStatusEndpoint(AsyncWebServer* server);
  private:
    void wifiStatusHandler(AsyncWebServerRequest* request);
};

#define WIFI_LIST_ENDPOINT_PATH "/api/savedNetworks"

class WiFiListSavedEndpoint {
 public:
  WiFiListSavedEndpoint(AsyncWebServer* server, FSPersistence *pFsp);

 private:
  void deleteNetwork(AsyncWebServerRequest* request);
  void listNetworks(AsyncWebServerRequest* request);
private:
  FSPersistence *_pFsp;
  std::vector<String> _savedNetworks;
};

// helpers
std::vector<String> getKnownWiFiNetworksNames(FSPersistence *pFS);
int getKnownWiFiNetworksCount(FSPersistence *pFS);
std::vector<WiFiNetwork> getConnectableNetworks(std::vector<String> saved);
void startScan();
int8_t checkScanResult() ;
void removeNetwork(FSPersistence *pFsp, const String &ssid);
void sendError(AsyncWebServerRequest* request, const String &err);

#endif //WS_WIFI_H
