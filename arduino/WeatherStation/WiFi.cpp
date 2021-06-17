#include "WiFi.h"
#include "IPUtils.h"

void printWifiSettings(String prefix, WiFiSettings *s) {
    Serial.print(prefix);
    Serial.print(F(" ssid: "));Serial.print(s->ssid);
    Serial.print(F(", password: "));Serial.print(s->password);
    Serial.print(F(", hostname: "));Serial.print(s->hostname);
    Serial.print(F(", static_ip_config: "));Serial.print(s->staticIPConfig);
    Serial.print(F(", local_ip: "));Serial.print(s->localIP);
    Serial.print(F(", gateway_ip: "));Serial.print(s->gatewayIP);
    Serial.print(F(", subnet_mask: "));Serial.print(s->subnetMask);
    Serial.print(F(", dns_ip_1: "));Serial.print(s->dnsIP1);
    Serial.print(F(", dns_ip_2: "));Serial.print(s->dnsIP2);
    Serial.println();
}

char *createDefaultHostname() {
    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    auto len = 13+strlen(WIFI_HOSTNAME_PREFIX)+1;
    char *hostname = new char[len];
    snprintf(hostname, len, WIFI_HOSTNAME_PREFIX "%02x%02x", mac[4], mac[5]);
    return hostname;
}

char *WiFiSettings::DefaultHostname = nullptr;

WiFiSettings::WiFiSettings():
  ssid(DEFAULT_WIFI_SSID),
  password(DEFAULT_WIFI_PASSWORD),
  staticIPConfig(false) { 
  if(!DefaultHostname) {
    DefaultHostname = createDefaultHostname();
  }
  hostname = DefaultHostname;
}

int WiFiSettings::save(JsonObject& root) {
    root[F("ssid")] = ssid;
    root[F("password")] = password;
    root[F("hostname")] = hostname;
    root[F("static_ip_config")] = staticIPConfig;

    writeIP(root, F("local_ip"), localIP);
    writeIP(root, F("gateway_ip"), gatewayIP);
    writeIP(root, F("subnet_mask"), subnetMask);
    writeIP(root, F("dns_ip_1"), dnsIP1);
    writeIP(root, F("dns_ip_2"), dnsIP2);
    printWifiSettings("Save WiFi settings", this);
    return 0;
}

int WiFiSettings::load(JsonObject& root) {
  ssid = root[F("ssid")].as<const char *>();
  password = root[F("password")].as<const char *>();
  hostname = root[F("hostname")].as<const char *>();
  staticIPConfig = root[F("static_ip_config")];

  // extended settings
  readIP(root, F("local_ip"), localIP);
  readIP(root, F("gateway_ip"), gatewayIP);
  readIP(root, F("subnet_mask"), subnetMask);
  readIP(root, F("dns_ip_1"), dnsIP1);
  readIP(root, F("dns_ip_2"), dnsIP2);

  // Swap dns servers if 2 is is set but 1 not
  if (!isIPSet(dnsIP1) && isIPSet(dnsIP2)) {
    dnsIP1 = dnsIP2;
    dnsIP2 = INADDR_NONE;
  }

  // Turn off static ip config if not full config is set: ipAddress, gateway and subnet
  if (staticIPConfig && (!isIPSet(localIP) || !isIPSet(gatewayIP) || !isIPSet(subnetMask))) {
    staticIPConfig = false;
  }
  printWifiSettings("Load WiFi settings", this);
  return 1;
}

// **************************  

WiFiManager::WiFiManager(WiFiSettings *settings):
  _settings(settings),
  _lastConnectionAttempt(0),
  _dnsServer(nullptr),
  _forceAPStop(false) {
#if 1    
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

  _settings->setHandler([this]() {
    Serial.println(F("Wifi settings changed, reconfigure"));
    reconfigureWiFiConnection();
    ESP.restart();
  });
#endif
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

void WiFiManager::notifyWifiEvent(WifiConnectionEvent event) {
  if(_wifiEventHandler) {
    _wifiEventHandler(event, _settings->ssid.c_str());
  }
}

void WiFiManager::notifyAPEvent() {
  if(_apEventHandler) {
    _apEventHandler(&_apInfo);
  }
}

void WiFiManager::loop() {
    unsigned long currentMillis = millis();
    unsigned long manageElapsed = (unsigned long)(currentMillis - _lastConnectionAttempt);
    if (!_lastConnectionAttempt || manageElapsed >= WIFI_RECONNECTION_DELAY) {
        _lastConnectionAttempt = currentMillis;
        manageSTA();
    }
    // try AP after only first conn attempt   
    if (_forceAPStop || manageElapsed >= WIFI_RECONNECTION_DELAY) {
        manageAP();
    }
    handleDNS();
}

void WiFiManager::end() {
  WiFi.disconnect(true);
}

void WiFiManager::manageSTA() {
    //Serial.printf("manageSTA: current wifi mode %d, status %d\n",  WiFi.getMode(), WiFi.status());
    // Abort if already connected, or if we have no SSID
    if (WiFi.isConnected()) {
        return;
    }
    if(!_settings->ssid.length()) {
        manageAP();
        return;
    }
    // Connect or reconnect as required
    if ((WiFi.getMode() & WIFI_STA) == 0) {
        Serial.println(F("Connecting to WiFi."));
        if (_settings->staticIPConfig) {
            // configure for static IP
            WiFi.config(_settings->localIP, _settings->gatewayIP, _settings->subnetMask, _settings->dnsIP1, _settings->dnsIP2);
        } else {
            // configure for DHCP
            WiFi.config(INADDR_ANY, INADDR_ANY, INADDR_ANY);
            WiFi.hostname(_settings->hostname);
        }
        // attempt to connect to the network
        WiFi.begin(_settings->ssid.c_str(), _settings->password.c_str());
        notifyWifiEvent(WifiConnectionEvent::ConnectingStarted);
    }
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
    Serial.println(F("Starting software access point"));
    Serial.println(F("   config: IP: " AP_LOCAL_IP " , GW IP: " AP_GATEWAY_IP ", Mask: " AP_SUBNET_MASK));
    Serial.printf_P(PSTR("         SSID: %s, Pass: " AP_PASSWORD "\n"), _apInfo.ssid.c_str());
    WiFi.softAPConfig(_apInfo.ipAddress, ipFromString(AP_GATEWAY_IP), ipFromString(AP_SUBNET_MASK));
    WiFi.softAP(_apInfo.ssid.c_str(), _apInfo.password.c_str(), AP_CHANNEL, AP_SSID_HIDDEN, AP_MAX_CLIENTS);
    if (!_dnsServer) {
        IPAddress apIp = WiFi.softAPIP();
        // TODO: to display
        Serial.print(F("Starting captive portal on "));
        Serial.println(apIp);
        _dnsServer = new DNSServer;
        _dnsServer->start(DNS_PORT, "*", apIp);
    }
    _apInfo.running = true;
    notifyAPEvent();
}

void WiFiManager::stopAP() {
    if (_dnsServer) {
        Serial.println(F("Stopping captive portal"));
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    Serial.println(F("Stopping software access point"));
    WiFi.softAPdisconnect(true);
    _apInfo.running = false;
    _forceAPStop = false;
    notifyAPEvent();
}

void WiFiManager::handleDNS() {
    if (_dnsServer) {
        _dnsServer->processNextRequest();
    }
}

void WiFiManager::onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  Serial.print(F("WiFi Disconnected. Reason code="));
  Serial.println(event.reason);
  WiFi.disconnect(true);
  if(event.reason < 200) {
    reconfigureWiFiConnection();
  }
  notifyWifiEvent(WifiConnectionEvent::ConnectingFailed);
}

void WiFiManager::onStationModeConnected(const WiFiEventStationModeConnected& event) {
  Serial.print(F("WiFi Connected. SSID="));
  Serial.println(event.ssid);
  notifyWifiEvent(WifiConnectionEvent::ConnectingSuccess);
  if(_apInfo.running) {
    _forceAPStop = true;
  }
}

void WiFiManager::onStationModeGotIP(const WiFiEventStationModeGotIP& event) {
  Serial.printf_P(PSTR("WiFi Got IP. localIP=%s, hostName=%s\n"), event.ip.toString().c_str(), WiFi.hostname().c_str());
}

#if 1

// ****************** WiFiScannerEndpoint ***************************

WiFiScannerEndpoint::WiFiScannerEndpoint(AsyncWebServer* server) {
  server->on(SCAN_NETWORKS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiScannerEndpoint::scanNetworks, this, std::placeholders::_1));
  server->on(LIST_NETWORKS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiScannerEndpoint::listNetworks, this, std::placeholders::_1));
};

void WiFiScannerEndpoint::scanNetworks(AsyncWebServerRequest* request) {
  if (WiFi.scanComplete() != -1) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
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

WiFiStatusEndpoint::WiFiStatusEndpoint(AsyncWebServer* server) {
  server->on(WIFI_STATUS_ENDPOINT_PATH, HTTP_GET, std::bind(&WiFiStatusEndpoint::wifiStatusHandler, this, std::placeholders::_1));
}

void WiFiStatusEndpoint::wifiStatusHandler(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  wl_status_t status = WiFi.status();
  root[F("mode")] = WiFi.getMode();
  root[F("status")] =status;
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
#endif
