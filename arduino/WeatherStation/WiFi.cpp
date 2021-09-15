#include "WiFi.h"
#include "IPUtils.h"

const char *StringId PROGMEM = "id";

WiFiManager::WiFiManager(FSPersistence *pFsp, WiFiSettings *pSettings):
  _pFsp(pFsp),
  _pSettings(pSettings) {
  // We want the device to come up in opmode=0 (WIFI_OFF), when erasing the flash, this is not the default.
  // If needed, we save opmode=0 before disabling persistence so the device boots with WiFi disabled in the future.
  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.mode(WIFI_OFF);
  }

  // Disable WiFi config persistance and auto reconnect
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  
  _onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected(std::bind(&WiFiManager::onStationModeDisconnected, this, std::placeholders::_1));
  _onStationModeConnectedHandler = WiFi.onStationModeConnected(std::bind(&WiFiManager::onStationModeConnected, this, std::placeholders::_1));
  _onStationModeGotIPHandler = WiFi.onStationModeGotIP(std::bind(&WiFiManager::onStationModeGotIP, this, std::placeholders::_1));
  _onSoftAPModeStationConnectedHandler = WiFi.onSoftAPModeStationConnected(std::bind(&WiFiManager::onSoftAPModeStationConnected, this, std::placeholders::_1));
  _onSoftAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(std::bind(&WiFiManager::onSoftAPModeStationDisconnected, this, std::placeholders::_1));

  _pSettings->setHandler([this]() {
    Serial.println(F("[WIFIM] Wifi settings changed, reconfigure"));
    // Reconfiguration will lead to disconnect which will prematurely mark testing failed
    _ignoreDisconnect = WiFi.isConnected();
    enterState(WiFiConnectingState::TestingConfig, true);
  });
}

void WiFiManager::reconfigureWiFiConnection() {
  // reset last connection attempt to force loop to reconnect immediately
  _lastConnectionAttempt = 0;
  _connectAttempts = 0;
  _lastDisconnectReason = 0;
  // disconnect and de-configure wifi
  WiFi.disconnect(true);
}

void WiFiManager::begin() {
  reconfigureWiFiConnection();
}

void WiFiManager::notifyWifiEvent(WifiConnectionEvent event) {
  if(_wifiEventHandler) {
    _wifiEventHandler(event, _pSettings->ssid.c_str());
  }
}

void WiFiManager::notifyAPEvent(WifiAPEvent event) {
  if(_apEventHandler) {
    _apEventHandler(event, &_apInfo);
  }
}

void WiFiManager::loop() {
    if(_asyncEventToFire) {
      notifyAPEvent(*_asyncEventToFire);
      delete _asyncEventToFire;
      _asyncEventToFire = nullptr;
    }
    uint32_t currentMillis = millis();
    uint32_t manageElapsed =_lastConnectionAttempt?(currentMillis - _lastConnectionAttempt):0;
    if (!_lastConnectionAttempt || manageElapsed >= _manageDelay) {
      manageSTA();
      _lastConnectionAttempt = currentMillis;
    }
    // try AP after only first conn attempt   
    if ((_forceAPStop &&  _forceAPStop <= millis()) || (!_forceAPStop &&  manageElapsed >= WIFI_RECONNECTION_DELAY)) {
        manageAP();
    }
    handleDNS();
}

void WiFiManager::end() {
  WiFi.disconnect(true);
}


void WiFiManager::loadSettings(const String &ssid) {
  _pSettings->setFilePath(F(WIFI_CONFIG_DIRECTORY "/") + ssid);
  _pFsp->readFromFS(_pSettings);
  //_pSettings.notify();
}

void WiFiManager::enterState(WiFiConnectingState newState, bool reconFigure) {
  _state = newState;
  if(reconFigure) {
    reconfigureWiFiConnection();
  }
}

void WiFiManager::manageSTA() {
  if(!_connectingToWifi) {
    switch(_state) {
      case WiFiConnectingState::NotConnected:
        {
          // we have no configured wifis, start AP directly
          int c = getKnownWiFiNetworksCount(_pFsp);
          Serial.printf_P(PSTR("[WIFIM] Found %d save networks\n"),c);
          if(!c) {
            manageAP();
            return;
          }
          notifyWifiEvent(WifiConnectionEvent::ConnectingStarted);
          startScan();
          _manageDelay = SCAN_NETWORK_DELAY;
          enterState(WiFiConnectingState::ScanStarted, false);
        }
        break;
      case WiFiConnectingState::ScanStarted:
        {
          int8_t scanres = checkScanResult();
          if(scanres == WIFI_SCAN_FAILED) {
            Serial.println(F("[WIFIM] Scan failed"));
            enterState(WiFiConnectingState::ScanFailed, true);
          } else if(scanres == WIFI_SCAN_RUNNING) {
            Serial.println(F("[WIFIM] Scan running"));
          } else {
            Serial.printf_P(PSTR("[WIFIM] found %d networks\n"), scanres);
            enterState(WiFiConnectingState::ScanFinished, true);
          }
        }
        break;
      case WiFiConnectingState::ScanFailed:
        _manageDelay = MANAGE_NETWORK_DELAY;
        enterState(WiFiConnectingState::NotConnected);
        break;;
      case WiFiConnectingState::ScanFinished:
        _savedNetworks = getKnownWiFiNetworksNames(_pFsp);
        _foundNetworks = getConnectableNetworks(_savedNetworks);
        _wifiNetworkIndex = 0;
        enterState(WiFiConnectingState::ConnectingToKnown, true);
        break;
      case WiFiConnectingState::ConnectingToKnown:
        if(_wifiNetworkIndex < _foundNetworks.size()) {
          WiFiNetwork *wn = &_foundNetworks[_wifiNetworkIndex];
          loadSettings(wn->ssid);
          startSTA(_pSettings, wn);
          _wifiNetworkIndex++;
        } else {
          // so far no successfull connection
          // try saved networks not found in scan
          _wifiNetworkIndex = 0;
          enterState(WiFiConnectingState::ConnectingToHidden);
        }
        break;
      case WiFiConnectingState::ConnectingToHidden:
again:        
        if(_wifiNetworkIndex < _savedNetworks.size()) {
          for(WiFiNetwork &wn : _foundNetworks) {
            if(wn.ssid == _savedNetworks[_wifiNetworkIndex]) {
              _wifiNetworkIndex++;
              goto again;
            }
          } 
          loadSettings(_savedNetworks[_wifiNetworkIndex]);
          startSTA(_pSettings);
          _wifiNetworkIndex++;
        } else {
          // not possible to found any connectable network
          Serial.println(F("[WIFIM] Not possible to connect"));
          _manageDelay = MANAGE_NETWORK_DELAY;
          enterState(WiFiConnectingState::NotConnected);
        }
        break;  
      case WiFiConnectingState::ConnectingSuccess:
        _manageDelay = MANAGE_NETWORK_DELAY;
        cleanNetworks();
        enterState(WiFiConnectingState::Idle);
        break;
      case WiFiConnectingState::Idle:
        // Do nothing
        break;
      case WiFiConnectingState::TestingConfig:
        Serial.println(F("[WIFIM] Testing config"));
        if(!startSTA(_pSettings)) {
          enterState(WiFiConnectingState::TestingConfigFailed);  
        }
        break;
      case WiFiConnectingState::TestingConfigFailed:
        Serial.println(F("[WIFIM] Testing config failed. removing"));
        _manageDelay = SCAN_NETWORK_DELAY;
        removeNetwork(_pFsp, _pSettings->ssid);
        enterState(WiFiConnectingState::NotConnected, true);
        break;
    }
  }
}

void WiFiManager::cleanNetworks() {
  std::for_each(_savedNetworks.begin(), _savedNetworks.end(), [](String &value){ value = (const char *)nullptr; });
  _savedNetworks.clear();
  _foundNetworks.clear();
}



bool WiFiManager::startSTA(WiFiSettings *pConfig, WiFiNetwork *pNetwork) {
  Serial.println(F("[WIFIM] Connecting to WiFi."));
  if(!pConfig->ssid.length()) {
    Serial.println(F("[WIFIM] error, empty ssid!"));
    return false;
  }
  if (pConfig->staticIPConfig) {
      // configure for static IP
      WiFi.config(pConfig->localIP, pConfig->gatewayIP, pConfig->subnetMask, pConfig->dnsIP1, pConfig->dnsIP2);
  } else {
      // configure for DHCP
      WiFi.config(INADDR_ANY, INADDR_ANY, INADDR_ANY);
      WiFi.hostname(pConfig->hostname);
  }
    // attempt to connect to the network
  if(pNetwork) {
    Serial.printf_P(PSTR("[WIFIM] Connecting to %s, chan %d\n"), pNetwork->ssid.c_str(), pNetwork->channel);
    WiFi.begin(pNetwork->ssid.c_str(), pConfig->password.c_str(), pNetwork->channel, pNetwork->bssid);  
  } else {
    Serial.printf_P(PSTR("[WIFIM] Connecting to %s\n"), pConfig->ssid.c_str());
    WiFi.begin(pConfig->ssid.c_str(), pConfig->password.c_str());
  }
  _connectingToWifi = true;
  _connectAttempts++;
  _lastDisconnectReason = 0;
  return true;
}

void WiFiManager::manageAP() {
    WiFiMode_t currentWiFiMode = WiFi.getMode();
    if ( WiFi.status() != WL_CONNECTED ) {
      if (currentWiFiMode == WIFI_OFF || currentWiFiMode == WIFI_STA) {
        startAP();
      }
    } else if (_forceAPStop || ((currentWiFiMode == WIFI_AP ||  currentWiFiMode == WIFI_AP_STA) && !WiFi.softAPgetStationNum())) {
        stopAP();
    }
}

char *createAPSSID() {
    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    auto len = 13+strlen(AP_SSID_PREFIX)+1;
    char *ssid = new char[len];
    snprintf(ssid, len, AP_SSID_PREFIX "%02x%02x", mac[4], mac[5]);
    return ssid;
}

void WiFiManager::startAP() {
    if(!_apInfo.ssid.length()) {
        char *ssid = createAPSSID();
        _apInfo.ssid = ssid;
        delete [] ssid;
        _apInfo.password = AP_PASSWORD;
        _apInfo.ipAddress = ipFromString(AP_LOCAL_IP);
    }
    _apInfo.clientsCount = 0;
    Serial.println(F("[WIFIM] Starting software access point"));
    Serial.println(F("[WIFIM]    config: IP: " AP_LOCAL_IP " , GW IP: " AP_GATEWAY_IP ", Mask: " AP_SUBNET_MASK));
    Serial.printf_P(PSTR("[WIFIM]          SSID: %s, Pass: " AP_PASSWORD "\n"), _apInfo.ssid.c_str());
    WiFi.softAPConfig(_apInfo.ipAddress, ipFromString(AP_GATEWAY_IP), ipFromString(AP_SUBNET_MASK));
    WiFi.softAP(_apInfo.ssid.c_str(), _apInfo.password.c_str(), AP_CHANNEL, AP_SSID_HIDDEN, AP_MAX_CLIENTS);
    if (!_dnsServer) {
        IPAddress apIp = WiFi.softAPIP();
        // TODO: to display
        Serial.print(F("[WIFIM] Starting captive portal on "));
        Serial.println(apIp);
        _dnsServer = new DNSServer;
        _dnsServer->start(DNS_PORT, "*", apIp);
    }
    _apInfo.running = true;
    notifyAPEvent(WifiAPEvent::APStarted);
}

void WiFiManager::stopAP() {
    if (_dnsServer) {
        Serial.println(F("[WIFIM] Stopping captive portal"));
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    Serial.println(F("[WIFIM] Stopping software access point"));
    WiFi.softAPdisconnect(true);
    _apInfo.running = false;
    _forceAPStop = 0;
    notifyAPEvent(WifiAPEvent::APStopped);
    WiFi.disconnect(true);
    ESP.restart();
}

void WiFiManager::handleDNS() {
    if (_dnsServer) {
        _dnsServer->processNextRequest();
    }
}

void WiFiManager::onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  Serial.print(F("[WIFIM] WiFi Disconnected. Reason code="));
  Serial.println(event.reason);
  _lastDisconnectReason = event.reason;
  WiFi.disconnect(true);
  //// speed up reconnecting
  // if(_connectAttempts < 3) {
  //   _lastConnectionAttempt = 0;
  // }
  _connectingToWifi = false;
  if(!_ignoreDisconnect) {
    switch(_state) {
      case WiFiConnectingState::ConnectingToKnown:
        if(_wifiNetworkIndex < _foundNetworks.size()) {
          _foundNetworks[_wifiNetworkIndex].connectSkip = true;
        } 
        break;
      case WiFiConnectingState::TestingConfig:
        enterState(WiFiConnectingState::TestingConfigFailed);
        break;
      case WiFiConnectingState::Idle:
        _manageDelay = SCAN_NETWORK_DELAY;
        enterState(WiFiConnectingState::NotConnected, true);
        break;
    }
    notifyWifiEvent(WifiConnectionEvent::ConnectingFailed);
  } else {
    _ignoreDisconnect = false;
  }
}

void WiFiManager::onStationModeConnected(const WiFiEventStationModeConnected& event) {
  Serial.print(F("[WIFIM] WiFi Connected. SSID="));
  Serial.println(event.ssid);
  _connectingToWifi = false;
  _state = WiFiConnectingState::ConnectingSuccess;
  notifyWifiEvent(WifiConnectionEvent::ConnectingSuccess);
  if(_apInfo.running) {
    // stop ap after a time, so connected client gets a HTTP reponse about successful connection
    _forceAPStop = millis()+10000;
  }
}

void WiFiManager::onStationModeGotIP(const WiFiEventStationModeGotIP& event) {
  Serial.printf_P(PSTR("[WIFIM] WiFi Got IP. localIP=%s, hostName=%s\n"), event.ip.toString().c_str(), WiFi.hostname().c_str());
}

void WiFiManager::onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& event) {
  Serial.println(F("[WIFIM] WiFi AP Client connected."));
  _apInfo.clientsCount++;
  // APevents must be propagated async
  _asyncEventToFire = new WifiAPEvent(WifiAPEvent::ClientConnected);
}

void WiFiManager::onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& event) {
  Serial.println(F("[WIFIM] WiFi AP Client disconnected."));
  _apInfo.clientsCount--;
  // APevents must be propagated async
  _asyncEventToFire = new WifiAPEvent(WifiAPEvent::ClientDisconnected);
}


// ****************** WiFiScannerEndpoint ***************************

WiFiScannerEndpoint::WiFiScannerEndpoint(AsyncWebServer* server) {
  server->on(SCAN_NETWORKS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiScannerEndpoint::scanNetworks, this, std::placeholders::_1));
  server->on(LIST_NETWORKS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiScannerEndpoint::listNetworks, this, std::placeholders::_1));
};

void WiFiScannerEndpoint::scanNetworks(AsyncWebServerRequest* request) {
  if (checkScanResult() != -1) {
    startScan();
  }
  request->send(202);
}

void WiFiScannerEndpoint::listNetworks(AsyncWebServerRequest* request) {
    int numNetworks = WiFi.scanComplete();
    if (numNetworks > -1) {
        AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
        JsonObject root = response->getRoot();
        JsonArray networks = root.createNestedArray(F("networks"));
        for (int i = 0; i < numNetworks; i++) {
            JsonObject network = networks.createNestedObject();
            network[F("rssi")] = WiFi.RSSI(i);
            network[F("ssid")] = WiFi.SSID(i);
            network[F("bssid")] = WiFi.BSSIDstr(i);
            network[F("channel")] = WiFi.channel(i);
            network[F("encryption_type")] = convertEncryptionType(WiFi.encryptionType(i));
        }
        response->setLength();
        request->send(response);
    } else if (numNetworks == -1) {
        request->send(202);
    } else {
        scanNetworks(request);
    }
}

/*
 * Convert auth mode to more standard encryption mode
 */
uint8_t WiFiScannerEndpoint::convertEncryptionType(uint8_t encryptionType) {
    switch (encryptionType) {
        case ENC_TYPE_NONE:
        return AUTH_OPEN;
        case ENC_TYPE_WEP:
        return AUTH_WEP;
        case ENC_TYPE_TKIP:
        return AUTH_WPA_PSK;
        case ENC_TYPE_CCMP:
        return AUTH_WPA2_PSK;
        case ENC_TYPE_AUTO:
        return AUTH_WPA_WPA2_PSK;
    }
    return -1;
}


// **************************** WiFiStatusEndpoint *********************************

WiFiStatusEndpoint::WiFiStatusEndpoint(AsyncWebServer* server, WiFiManager *wifiManager):_wifiManager(wifiManager) {
  server->on(WIFI_STATUS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiStatusEndpoint::wifiStatusHandler, this, std::placeholders::_1));
}

void WiFiStatusEndpoint::wifiStatusHandler(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  wl_status_t status = WiFi.status();
  root[F("mode")] = WiFi.getMode();
  root[F("status")] = status;
  if(_wifiManager->getLastDisconnectReason()) {
    root[F("disconnect_reason")] = _wifiManager->getLastDisconnectReason();
  }
  if (status == WL_CONNECTED) {
    root[F("local_ip")] = WiFi.localIP().toString();
    root[F("mac_address")] = WiFi.macAddress();
    root[F("rssi")] = WiFi.RSSI();
    root[F("ssid")] = WiFi.SSID();
    root[F("bssid")] = WiFi.BSSIDstr();
    root[F("channel")] = WiFi.channel();
    root[F("subnet_mask")] = WiFi.subnetMask().toString();
    root[F("gateway_ip")] = WiFi.gatewayIP().toString();
    IPAddress dnsIP1 = WiFi.dnsIP(0);
    IPAddress dnsIP2 = WiFi.dnsIP(1);
    if (isIPSet(dnsIP1)) {
      root[F("dns_ip_1")] = dnsIP1.toString();
    }
    if (isIPSet(dnsIP2)) {
      root[F("dns_ip_2")] = dnsIP2.toString();
    }
  }
  response->setLength();
  request->send(response);
}

WiFiListSavedEndpoint::WiFiListSavedEndpoint(AsyncWebServer* server, FSPersistence *pFsp):
_pFsp(pFsp) {
  server->on(WIFI_LIST_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiListSavedEndpoint::listNetworks, this, std::placeholders::_1));
  server->on(WIFI_LIST_ENDPOINT_PATH, HTTP_DELETE, std::bind(&WiFiListSavedEndpoint::deleteNetwork, this, std::placeholders::_1));
};


void WiFiListSavedEndpoint::listNetworks(AsyncWebServerRequest* request) {
  _savedNetworks = getKnownWiFiNetworksNames(_pFsp);
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  JsonArray networks = root.createNestedArray(F("networks"));
  for (int i = 0; i < _savedNetworks.size(); i++) {
      JsonObject network = networks.createNestedObject();
      network[FPSTR(StringId)] = i+1;
      network[F("name")] = _savedNetworks[i];
      network[F("connected")] = _savedNetworks[i] == WiFi.SSID();
  }
  response->setLength();
  request->send(response);
}

void WiFiListSavedEndpoint::deleteNetwork(AsyncWebServerRequest* request) {
  if(request->hasParam(FPSTR(StringId))) {
    AsyncWebParameter *idp = request->getParam(FPSTR(StringId));
    Serial.printf_P(PSTR(" deleteNetwork id %s\n"), idp->value().c_str());
    int id = idp->value().toInt();
    if(id > 0 ) {
      removeNetwork(_pFsp, _savedNetworks[id-1]);
      if(WiFi.SSID() == _savedNetworks[id-1]) {
        request->onDisconnect([](){
          WiFi.disconnect();
        });  
      }
      listNetworks(request);
    } else {
      String err = F("Invalid id ");
      err += idp->value();      
      sendError(request, err);
    }
  } else {
    String err = F("Missing param id ");
    sendError(request, err);
  }
}

void sendError(AsyncWebServerRequest* request, const String &err) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  root[F("error")] = err;
  response->setCode(400);
  response->setLength();
  request->send(response);
}

std::vector<String> getKnownWiFiNetworksNames(FSPersistence *pFS) {
  return pFS->listConfigs(WIFI_CONFIG_DIRECTORY, true);
}

int getKnownWiFiNetworksCount(FSPersistence *pFS) {
  std::vector<String> files = pFS->listConfigs(WIFI_CONFIG_DIRECTORY, true);
  return files.size();
}

void startScan()
{
  Serial.println(F("[WIFIM] Scan starting"));

  // Clean previous scan
  WiFi.scanDelete();

  // Start wifi scan in async mode
  WiFi.scanNetworks(true);
}

int8_t checkScanResult() {
  return WiFi.scanComplete();
}

 
std::vector<WiFiNetwork> getConnectableNetworks(std::vector<String> saved)
{
    int8_t scanResult;
    String ssid;
    int32_t rssi;
    uint8_t encType;
    uint8_t *bssid;
    int32_t channel;
    bool hidden;

    std::vector<WiFiNetwork>  found;

    // Get scan results
    scanResult = WiFi.scanComplete();

    // Find known WiFi networks
    uint8_t known[saved.size()];
    uint8_t numNetworks = 0;
    for (int8_t i = 0; i < scanResult; i++) {
        // Get network information
        WiFi.getNetworkInfo(i, ssid, encType, rssi, bssid, channel, hidden);
        Serial.printf_P(PSTR("[WIFIM] Scan found %s, chan %d, rssi %d\n"), ssid.c_str(), channel, rssi);
        // Check if the WiFi network contains an entry in AP list
        for (auto entry : saved) {
            // Check SSID
            if (ssid == entry) {
                // Known network
                known[numNetworks++] = i;
            }
        }
    }

    // Sort WiFi networks by RSSI
    for (int i = 0; i < numNetworks; i++) {
        for (int j = i + 1; j < numNetworks; j++) {
            if (WiFi.RSSI(known[j]) > WiFi.RSSI(known[i])) {
                int8_t tmp;

                // Swap indices
                tmp = known[i];
                known[i] = known[j];
                known[j] = tmp;
            }
        }
    }

    // // Print sorted indices
    // Serial.println(F("[WIFIM] Sorted indices: "));
    // for (int8_t i = 0; i < numNetworks; i++) {
    //     Serial.println(F("  %d "), known[i]);
    // }
    // Serial.println(F("\n"));

      // Connect to known WiFi AP's sorted by RSSI
    for (int8_t i = 0; i < numNetworks; i++) {
        // Get network information
        WiFi.getNetworkInfo(known[i], ssid, encType, rssi, bssid, channel, hidden);
        Serial.printf_P(PSTR("[WIFIM] Scan adding %s, chan %d, rssi %d\n"), ssid.c_str(), channel, rssi);
        WiFiNetwork wn = {ssid, bssid, rssi, channel, false };
        found.push_back(wn);
    }
    return found;
} 

void removeNetwork(FSPersistence *pFsp, const String &ssid) {
  pFsp->removeConfig(F(WIFI_CONFIG_DIRECTORY "/") + ssid);
}
