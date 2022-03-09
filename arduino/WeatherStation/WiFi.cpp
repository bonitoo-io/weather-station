#include "WiFi.h"
#include "IPUtils.h"

const char *StringId PROGMEM = "id";

static char *copyChars(const char *str) {
  char *ret = new char[strlen(str)+1];
  strcpy(ret, str);
  return ret;
}

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
  
  _onStationModeDisconnectedHandler =   WiFi.onStationModeDisconnected(std::bind(&WiFiManager::onStationModeDisconnected, this, std::placeholders::_1));
  _onStationModeConnectedHandler = WiFi.onStationModeConnected(std::bind(&WiFiManager::onStationModeConnected, this, std::placeholders::_1));
  _onStationModeGotIPHandler = WiFi.onStationModeGotIP(std::bind(&WiFiManager::onStationModeGotIP, this, std::placeholders::_1));
  _onSoftAPModeStationConnectedHandler = WiFi.onSoftAPModeStationConnected(std::bind(&WiFiManager::onSoftAPModeStationConnected, this, std::placeholders::_1));
  _onSoftAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(std::bind(&WiFiManager::onSoftAPModeStationDisconnected, this, std::placeholders::_1));

  _pSettings->setHandler([this]() {
    Serial.println(F("[WIFIM] Wifi settings changed, reconfigure"));
    // Reconfiguration will lead to disconnect which will prematurely mark testing failed
    _ignoreDisconnect = WiFi.isConnected();
    if(WiFi.isConnected()) {
      // Remember previously connected network
      setPreviousNetwork(WiFi.SSID().c_str());
    }
    _connectTestSuccess = false;
    _manageDelay = SCAN_NETWORK_DELAY;
    // Config is changed by posting to endpoint
    enterState(WiFiConnectingState::TestingConfig, true);
  });
}

void WiFiManager::reconfigureWiFiConnection() {
  // reset last connection attempt to force loop to reconnect immediately
  _lastConnectionAttempt = 0;
  // disconnect and de-configure wifi
  WiFi.disconnect(true);
}

void WiFiManager::begin() {
  reconfigureWiFiConnection();
}

void WiFiManager::setPreviousNetwork(const char *network) {
  if(_previousNetwork) {
    delete [] _previousNetwork;
    _previousNetwork = nullptr;
  }
  if(network) {
    _previousNetwork = copyChars(network);
  }
}

void WiFiManager::notifyWifiEvent(WifiConnectionEvent event) {
  if(_wifiEventHandler) {
    _wifiEventHandler(event, _pSettings->ssid.c_str());
  }
}

void WiFiManager::notifyAPEvent(WifiAPEvent event) {
  if(_apEventHandler) {
    _apEventHandler(event, _pApInfo);
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
    if (manageElapsed >= WIFI_RECONNECTION_DELAY) {
        manageAP(false);
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
    Serial.printf_P(PSTR("[WIFIM] state %d\n"), _state);
    switch(_state) {
      case WiFiConnectingState::NotConnected:
        {
          if(_previousNetwork) {
            Serial.printf_P(PSTR("[WIFIM] Connecting to last connected: %s\n"),_previousNetwork);
            loadSettings(_previousNetwork);
            setPreviousNetwork(nullptr);
            startSTA(_pSettings);
            return;
          }
          
          int c = getKnownWiFiNetworksCount(_pFsp);
          Serial.printf_P(PSTR("[WIFIM] Found %d save networks\n"),c);
          _firstStart = c == 0;
          if(!c) {
            // we have no configured wifis, start AP directly
            manageAP(false);
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
        if(_pApInfo) {
          Serial.printf_P(PSTR("[WIFIM] Managing AP. Force: %s\n"), _pApInfo->forceAPStop?"true":"false");
          if(_pApInfo->forceAPStop) {
            manageAP(true);
          } else {
            // Sth happend shutdown AP on next cycle
            _pApInfo->forceAPStop = true;
            _manageDelay = AP_SHUTDOWN_DELAY;
            enterState(WiFiConnectingState::ConnectingSuccess);
          }
        }
        break;
      case WiFiConnectingState::Idle:
        // Do nothing
        break;
      case WiFiConnectingState::TestingConfig:
        Serial.println(F("[WIFIM] Testing config"));
        _lastDisconnectReason = 0;
        if(!startSTA(_pSettings)) {
          enterState(WiFiConnectingState::TestingConfigFailed);  
        }
        break;
      case WiFiConnectingState::SaveConfig:
        Serial.println(F("[WIFIM] Saving WiFi config"));
        _pFsp->writeToFS(_pSettings);
        enterState(WiFiConnectingState::ConnectingSuccess);
        break;
      break;
      case WiFiConnectingState::TestingConfigFailed:
        Serial.println(F("[WIFIM] Testing config failed."));
        _manageDelay = getKnownWiFiNetworksCount(_pFsp)?SCAN_NETWORK_DELAY:MANAGE_NETWORK_DELAY;
        enterState(WiFiConnectingState::NotConnected, true);
        break;
      case WiFiConnectingState::ConnectingToSaved:
        loadSettings(_savedNetworks[_wifiNetworkIndex]);
        _lastDisconnectReason = 0;
        startSTA(_pSettings);
        _wifiNetworkIndex = 0;
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
  notifyWifiEvent(WifiConnectionEvent::ConnectingUpdate);
    // attempt to connect to the network
  if(pNetwork) {
    Serial.printf_P(PSTR("[WIFIM] Connecting to %s, chan %d\n"), pNetwork->ssid.c_str(), pNetwork->channel);
    WiFi.begin(pNetwork->ssid.c_str(), pConfig->password.c_str(), pNetwork->channel, pNetwork->bssid);  
  } else {
    Serial.printf_P(PSTR("[WIFIM] Connecting to %s\n"), pConfig->ssid.c_str());
    WiFi.begin(pConfig->ssid.c_str(), pConfig->password.c_str());
  }
  _connectingToWifi = true;
  return true;
}

void WiFiManager::manageAP(bool forceStop) {
    WiFiMode_t currentWiFiMode = WiFi.getMode();
    if ( WiFi.status() != WL_CONNECTED ) {
      if (currentWiFiMode == WIFI_OFF || currentWiFiMode == WIFI_STA) {
        startAP();
      }
    } else if (forceStop || ((currentWiFiMode == WIFI_AP ||  currentWiFiMode == WIFI_AP_STA) && !WiFi.softAPgetStationNum())) {
        stopAP();
    }
}

char *createAPSSID() {
    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    auto len = 13+strlen_P(PSTR(AP_SSID_PREFIX))+1;
    char *ssid = new char[len];
    snprintf_P(ssid, len, PSTR(AP_SSID_PREFIX "%02x%02x"), mac[4], mac[5]);
    return ssid;
}

void WiFiManager::startAP() {
    if(!_pApInfo) {
        _pApInfo = new APInfo;
        char *ssid = createAPSSID();
        _pApInfo->ssid = ssid;
        delete [] ssid;
        _pApInfo->password = AP_PASSWORD;
        _pApInfo->ipAddress = ipFromString(AP_LOCAL_IP);
    }
    _pApInfo->clientsCount = 0;
    Serial.println(F("[WIFIM] Starting software access point"));
    Serial.println(F("[WIFIM]    config: IP: " AP_LOCAL_IP " , GW IP: " AP_GATEWAY_IP ", Mask: " AP_SUBNET_MASK));
    Serial.printf_P(PSTR("[WIFIM]          SSID: %s, Pass: " AP_PASSWORD "\n"), _pApInfo->ssid.c_str());
    WiFi.softAPConfig(_pApInfo->ipAddress, ipFromString(AP_GATEWAY_IP), ipFromString(AP_SUBNET_MASK));
    WiFi.softAP(_pApInfo->ssid.c_str(), _pApInfo->password.c_str(), AP_CHANNEL, AP_SSID_HIDDEN, AP_MAX_CLIENTS);
    if (!_dnsServer) {
        IPAddress apIp = WiFi.softAPIP();
        // TODO: to display
        Serial.print(F("[WIFIM] Starting captive portal on "));
        Serial.println(apIp);
        _dnsServer = new DNSServer;
        _dnsServer->start(DNS_PORT, "*", apIp);
    }
    _pApInfo->running = true;
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
    _pApInfo->running = false;
    _pApInfo->forceAPStop = false;
    notifyAPEvent(WifiAPEvent::APStopped);
    delete _pApInfo;
    _pApInfo = nullptr;
    if(_firstStart) {
      WiFi.disconnect(true);
      delay(500);
      ESP.restart();
    }
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
  _connectingToWifi = false;
  ++_disconnectsCount;
  if(!_ignoreDisconnect) {
    switch(_state) {
      case WiFiConnectingState::ConnectingToKnown:
        if(_wifiNetworkIndex < _foundNetworks.size()) {
          _foundNetworks[_wifiNetworkIndex].connectSkip = true;
        } 
        break;
      case WiFiConnectingState::TestingConfig:
        enterState(WiFiConnectingState::TestingConfigFailed, true);
        break;
      case WiFiConnectingState::Idle:
      case WiFiConnectingState::ConnectingSuccess:
      case WiFiConnectingState::ConnectingToSaved:
        _manageDelay = SCAN_NETWORK_DELAY;
        enterState(WiFiConnectingState::NotConnected, true);
        break;
      default:
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
  _lastDisconnectReason = 0;
  if(_state == WiFiConnectingState::TestingConfig || _state == WiFiConnectingState::ConnectingToSaved ) {
    // Mark test success only when comming from testing states
    _connectTestSuccess = true;
  }
  if(_state == WiFiConnectingState::TestingConfig) {
    enterState(WiFiConnectingState::SaveConfig);  
  } else {
    enterState(WiFiConnectingState::ConnectingSuccess);
  }
  notifyWifiEvent(WifiConnectionEvent::ConnectingSuccess);
}

void WiFiManager::statusResponseSent() {
  if(_connectTestSuccess && _pApInfo && _pApInfo->running) {
    // stop AP (and restart) after successfull connection
    // wait a bit to have a user notice a notification
    Serial.println(F("[WIFIM] Success confirmed to UI"));
    _pApInfo->forceAPStop = true;
  }
}

void WiFiManager::onStationModeGotIP(const WiFiEventStationModeGotIP& event) {
  Serial.printf_P(PSTR("[WIFIM] WiFi Got IP. localIP=%s, hostName=%s\n"), event.ip.toString().c_str(), WiFi.hostname().c_str());
}

void WiFiManager::onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& event) {
  Serial.print(F("[WIFIM] WiFi AP Client connected: "));
  Serial.printf_P(PSTR("%02x:%02x:%02x:%02x:%02x:%02x\n"), event.mac[0], event.mac[1],event.mac[2], event.mac[3],event.mac[4], event.mac[5]);
  _pApInfo->clientsCount++;
  // APevents must be propagated async
  _asyncEventToFire = new WifiAPEvent(WifiAPEvent::ClientConnected);
}

void WiFiManager::onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& event) {
  Serial.print(F("[WIFIM] WiFi AP Client disconnected: "));
  Serial.printf_P(PSTR("%02x:%02x:%02x:%02x:%02x:%02x\n"), event.mac[0], event.mac[1],event.mac[2], event.mac[3],event.mac[4], event.mac[5]);
  if(_pApInfo) {
     _pApInfo->clientsCount--;
  }
  // APevents must be propagated async
  _asyncEventToFire = new WifiAPEvent(WifiAPEvent::ClientDisconnected);
}

void WiFiManager::connectToSavedNetwork(int index) {
  //refresh saved networks
  _savedNetworks = getKnownWiFiNetworksNames(_pFsp);
  Serial.printf_P(PSTR("[WIFIM]  Connect to saved %s\n"),_savedNetworks[index].c_str());
  _wifiNetworkIndex = index;
  // connecting to another network requires disconnect
  // ignore first disconnect event to avoid premature failure
  _ignoreDisconnect = true;
  setPreviousNetwork(WiFi.SSID().c_str());
  _connectTestSuccess = false;
  enterState(WiFiConnectingState::ConnectingToSaved, true);
}

// ****************** WiFiConnectionHelperEndpoint ***************************

WiFiConnectionHelperEndpoint::WiFiConnectionHelperEndpoint(AsyncWebServer* pServer, WiFiManager *pWiFiManager)
:_pWiFiManager(pWiFiManager) {
  AsyncCallbackJsonWebHandler *pConnectHandler = new AsyncCallbackJsonWebHandler(F(CONNECT_TO_SAVED_ENDPOINT_PATH), 
                std::bind(&WiFiConnectionHelperEndpoint::connectToSaved, this, std::placeholders::_1, std::placeholders::_2),
                DEFAULT_BUFFER_SIZE);
    pConnectHandler->setMethod(HTTP_POST);
    pServer->addHandler(pConnectHandler);
    pServer->on(CONNECT_STATUS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiConnectionHelperEndpoint::connectingStatus, this, std::placeholders::_1));
}

void WiFiConnectionHelperEndpoint::connectToSaved(AsyncWebServerRequest* request, JsonVariant& json) {
   if (!json.is<JsonObject>()) {
        request->send(400);
        return;
    }
    JsonObject jsonObject = json.as<JsonObject>();
    int id = jsonObject[FPSTR(StringId)].as<int>(); 
    if(id < 1) {
      String err = F("Invalid id ");
      err += String(id);      
      sendError(request, err);
      return;
    }
    --id;
    request->onDisconnect([this, id](){
      _pWiFiManager->connectToSavedNetwork(id);
    });
    request->send(202);
}

void WiFiConnectionHelperEndpoint::connectingStatus(AsyncWebServerRequest* request) {
  if(_pWiFiManager->isConnectingToWiFi()) {
    request->send(202);
  } else {
    AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
    JsonObject root = response->getRoot();
    root[F("success")] = _pWiFiManager->isConnectTestSuccessful();
    if(_pWiFiManager->getLastDisconnectReason()) {
      root[F("disconnect_reason")] = _pWiFiManager->getLastDisconnectReason();
    }
    response->addHeader(F("Cache-Control"),F("No-Store"));
    response->setLength();
    request->onDisconnect([this](){
      _pWiFiManager->statusResponseSent();
    });
    request->send(response);
  }
}

// ****************** WiFiScannerEndpoint ***************************

WiFiScannerEndpoint::WiFiScannerEndpoint(AsyncWebServer* server) {
  server->on(F(SCAN_NETWORKS_ENDPOINT_PATH), HTTP_GET, std::bind(&WiFiScannerEndpoint::scanNetworks, this, std::placeholders::_1));
  server->on(F(LIST_NETWORKS_ENDPOINT_PATH), HTTP_GET, std::bind(&WiFiScannerEndpoint::listNetworks, this, std::placeholders::_1));
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
            network[FPSTR(StringSSID)] = WiFi.SSID(i);
            network[F("bssid")] = WiFi.BSSIDstr(i);
            network[F("channel")] = WiFi.channel(i);
            network[F("encryption_type")] = convertEncryptionType(WiFi.encryptionType(i));
        }
        response->addHeader(F("Cache-Control"),F("No-Store"));
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

WiFiStatusEndpoint::WiFiStatusEndpoint(AsyncWebServer* server) {
  server->on(F(WIFI_STATUS_ENDPOINT_PATH), HTTP_GET, std::bind(&WiFiStatusEndpoint::wifiStatusHandler, this, std::placeholders::_1));
}

void WiFiStatusEndpoint::wifiStatusHandler(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  wl_status_t status = WiFi.status();
  root[F("mode")] = WiFi.getMode();
  root[F("status")] = status;
  if (status == WL_CONNECTED) {
    root[F("local_ip")] = WiFi.localIP().toString();
    root[F("mac_address")] = WiFi.macAddress();
    root[F("rssi")] = WiFi.RSSI();
    root[FPSTR(StringSSID)] = WiFi.SSID();
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
  response->addHeader(F("Cache-Control"),F("No-Store"));
  response->setLength();
  request->send(response);
}

// **************************** WiFiListSavedEndpoint *********************************

WiFiListSavedEndpoint::WiFiListSavedEndpoint(AsyncWebServer* server, FSPersistence *pFsp):
_pFsp(pFsp) {
  server->on(F(WIFI_LIST_ENDPOINT_PATH), HTTP_GET, std::bind(&WiFiListSavedEndpoint::listNetworks, this, std::placeholders::_1));
  server->on(F(WIFI_LIST_ENDPOINT_PATH), HTTP_DELETE, std::bind(&WiFiListSavedEndpoint::deleteNetwork, this, std::placeholders::_1));
};


void WiFiListSavedEndpoint::listNetworks(AsyncWebServerRequest* request) {
  _savedNetworks = getKnownWiFiNetworksNames(_pFsp);
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  JsonArray networks = root.createNestedArray(F("networks"));
  for (size_t i = 0; i < _savedNetworks.size(); i++) {
      JsonObject network = networks.createNestedObject();
      network[FPSTR(StringId)] = i+1;
      network[FPSTR(StringSSID)] = _savedNetworks[i];
      network[F("connected")] = _savedNetworks[i] == WiFi.SSID();
  }
  response->addHeader(F("Cache-Control"),F("No-Store"));
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
  return pFS->listConfigs(F(WIFI_CONFIG_DIRECTORY), true);
}

int getKnownWiFiNetworksCount(FSPersistence *pFS) {
  std::vector<String> files = pFS->listConfigs(F(WIFI_CONFIG_DIRECTORY), true);
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

