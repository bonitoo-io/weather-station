#include "WiFi.h"
#include "IPUtils.h"

void printWifiSettings(String prefix, WiFiSettings *s) {
    Serial.print(prefix);
    Serial.print(" ssid: ");Serial.print(s->ssid);
    Serial.print(", password: ");Serial.print(s->password);
    Serial.print(", hostname: ");Serial.print(s->hostname);
    Serial.print(", static_ip_config: ");Serial.print(s->staticIPConfig);
    Serial.print(", local_ip: ");Serial.print(s->localIP);
    Serial.print(", gateway_ip: ");Serial.print(s->gatewayIP);
    Serial.print(", subnet_mask: ");Serial.print(s->subnetMask);
    Serial.print(", dns_ip_1: ");Serial.print(s->dnsIP1);
    Serial.print(", dns_ip_2: ");Serial.print(s->dnsIP2);
    Serial.println();
}

int WiFiSettings::save(JsonObject& root) {
    root["ssid"] = ssid;
    root["password"] = password;
    root["hostname"] = hostname;
    root["static_ip_config"] = staticIPConfig;

    writeIP(root, "local_ip", localIP);
    writeIP(root, "gateway_ip", gatewayIP);
    writeIP(root, "subnet_mask", subnetMask);
    writeIP(root, "dns_ip_1", dnsIP1);
    writeIP(root, "dns_ip_2", dnsIP2);
    printWifiSettings("Save", this);
    return 0;
}

char *createDefaultHostname() {
    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    char *hostname = new char[13+strlen(WIFI_HOSTNAME_PREFIX)+1];
    sprintf(hostname, WIFI_HOSTNAME_PREFIX "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return hostname;
}

char *DefaultHostname = nullptr;

int WiFiSettings::load(JsonObject& root) {
    if(!DefaultHostname) {
        DefaultHostname = createDefaultHostname();
    }
    ssid = root["ssid"] | DEFAULT_WIFI_SSID;
    password = root["password"] | DEFAULT_WIFI_PASSWORD;
    hostname = root["hostname"] | DefaultHostname;
    staticIPConfig = root["static_ip_config"] | false;

    // extended settings
    readIP(root, "local_ip", localIP);
    readIP(root, "gateway_ip", gatewayIP);
    readIP(root, "subnet_mask", subnetMask);
    readIP(root, "dns_ip_1", dnsIP1);
    readIP(root, "dns_ip_2", dnsIP2);

    // Swap dns servers if 2 is is set but 1 not
    if (!isIPSet(dnsIP1) && isIPSet(dnsIP2)) {
        dnsIP1 = dnsIP2;
        dnsIP2 = INADDR_NONE;
    }

    // Turn off static ip config if not full config is set: ipAddress, gateway and subnet
    if (staticIPConfig && (!isIPSet(localIP) || !isIPSet(gatewayIP) ||
                                    !isIPSet(subnetMask))) {
         staticIPConfig = false;
    }
    printWifiSettings("Load", this);
    return 1;
}

WiFiStationService::WiFiStationService(WiFiSettings *settings):
    _settings(settings),
    _lastConnectionAttempt(0) {
    // We want the device to come up in opmode=0 (WIFI_OFF), when erasing the flash this is not the default.
    // If needed, we save opmode=0 before disabling persistence so the device boots with WiFi disabled in the future.
    if (WiFi.getMode() != WIFI_OFF) {
        WiFi.mode(WIFI_OFF);
    }

    // Disable WiFi config persistance and auto reconnect
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    _onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected(std::bind(&WiFiStationService::onStationModeDisconnected, this, std::placeholders::_1));


    _settings->setHandler(std::bind(&WiFiStationService::reconfigureWiFiConnection, this));
}

void WiFiStationService::reconfigureWiFiConnection() {
    Serial.println(F("ReconfigureWiFiConnection."));
    // reset last connection attempt to force loop to reconnect immediately
    _lastConnectionAttempt = 0;

    // disconnect and de-configure wifi
    WiFi.disconnect(true);
}

void WiFiStationService::begin() {
    reconfigureWiFiConnection();
}

void WiFiStationService::loop() {
    unsigned long currentMillis = millis();
    if (!_lastConnectionAttempt || (unsigned long)(currentMillis - _lastConnectionAttempt) >= WIFI_RECONNECTION_DELAY) {
        _lastConnectionAttempt = currentMillis;
        manageSTA();
    }
}

void WiFiStationService::manageSTA() {
    //Serial.printf("manageSTA: current wifi mode %d, status %d\n",  WiFi.getMode(), WiFi.status());
    // Abort if already connected, or if we have no SSID
    if (WiFi.isConnected() || _settings->ssid.length() == 0) {
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
    }
}


void WiFiStationService::onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  WiFi.disconnect(true);
}



// **************************** WifiAccessPointService *****************
char *createAPSSID() {
    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    char *ssid = new char[13+strlen(AP_SSID_PREFIX)+1];
    sprintf(ssid, AP_SSID_PREFIX "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return ssid;
}

char *APSSID = nullptr;

WifiAccessPointService::WifiAccessPointService() :
    _dnsServer(nullptr),
    _lastManaged(0),
    _reconfigureAp(false) {
}

void WifiAccessPointService::begin() {
    reconfigureAP();
}

void WifiAccessPointService::reconfigureAP() {
    _lastManaged = millis() - WIFI_RECONNECTION_DELAY;
    _reconfigureAp = true;
}

void WifiAccessPointService::loop() {
    unsigned long currentMillis = millis();
    unsigned long manageElapsed = (unsigned long)(currentMillis - _lastManaged);
    if (manageElapsed >= WIFI_RECONNECTION_DELAY) {
        _lastManaged = currentMillis;
        manageAP();
    }
    handleDNS();
}

void WifiAccessPointService::manageAP() {
    WiFiMode_t currentWiFiMode = WiFi.getMode();
    //Serial.printf("manageAP: current wifi mode %d, status %d\n", currentWiFiMode, WiFi.status());
    if ( WiFi.status() != WL_CONNECTED) {
        if (_reconfigureAp || currentWiFiMode == WIFI_OFF || currentWiFiMode == WIFI_STA) {
            startAP();
        }
    } else if ((currentWiFiMode == WIFI_AP || currentWiFiMode == WIFI_AP_STA) &&
                (_reconfigureAp || !WiFi.softAPgetStationNum())) {
        stopAP();
    }
    _reconfigureAp = false;
}


void WifiAccessPointService::startAP() {
    if(!APSSID) {
        APSSID = createAPSSID();
    }
    Serial.println(F("Starting software access point"));
    Serial.println(F("   config: IP: " AP_LOCAL_IP " , GW IP: " AP_GATEWAY_IP ", Mask: " AP_SUBNET_MASK));
    Serial.printf_P(PSTR("         SSID: %s, Pass: " AP_PASSWORD "\n"), APSSID);
    WiFi.softAPConfig(ipFromString(AP_LOCAL_IP), ipFromString(AP_GATEWAY_IP), ipFromString(AP_SUBNET_MASK));
    WiFi.softAP(APSSID, AP_PASSWORD, AP_CHANNEL, AP_SSID_HIDDEN, AP_MAX_CLIENTS);
    if (!_dnsServer) {
        IPAddress apIp = WiFi.softAPIP();
        // TODO: to display
        Serial.print(F("Starting captive portal on "));
        Serial.println(apIp);
        _dnsServer = new DNSServer;
        _dnsServer->start(DNS_PORT, "*", apIp);
    }
}

void WifiAccessPointService::stopAP() {
    if (_dnsServer) {
        Serial.println(F("Stopping captive portal"));
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    Serial.println(F("Stopping software access point"));
    WiFi.softAPdisconnect(true);
}

void WifiAccessPointService::handleDNS() {
    if (_dnsServer) {
        _dnsServer->processNextRequest();
    }
}

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
        JsonArray networks = root.createNestedArray("networks");
        for (int i = 0; i < numNetworks; i++) {
            JsonObject network = networks.createNestedObject();
            network["rssi"] = WiFi.RSSI(i);
            network["ssid"] = WiFi.SSID(i);
            network["bssid"] = WiFi.BSSIDstr(i);
            network["channel"] = WiFi.channel(i);
            network["encryption_type"] = convertEncryptionType(WiFi.encryptionType(i));
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
  _onStationModeConnectedHandler = WiFi.onStationModeConnected(onStationModeConnected);
  _onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected(onStationModeDisconnected);
  _onStationModeGotIPHandler = WiFi.onStationModeGotIP(onStationModeGotIP);
}

void WiFiStatusEndpoint::onStationModeConnected(const WiFiEventStationModeConnected& event) {
  Serial.print(F("WiFi Connected. SSID="));
  Serial.println(event.ssid);
}

void WiFiStatusEndpoint::onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  Serial.print(F("WiFi Disconnected. Reason code="));
  Serial.println(event.reason);
}

void WiFiStatusEndpoint::onStationModeGotIP(const WiFiEventStationModeGotIP& event) {
  Serial.printf_P(
      PSTR("WiFi Got IP. localIP=%s, hostName=%s\r\n"), event.ip.toString().c_str(), WiFi.hostname().c_str());
}



void WiFiStatusEndpoint::wifiStatusHandler(AsyncWebServerRequest* request) {
  AsyncJsonResponse* response = new AsyncJsonResponse(false, DEFAULT_BUFFER_SIZE);
  JsonObject root = response->getRoot();
  wl_status_t status = WiFi.status();
  root["mode"] = WiFi.getMode();
  root["status"] =status;
  if (status == WL_CONNECTED) {
    root["local_ip"] = WiFi.localIP().toString();
    root["mac_address"] = WiFi.macAddress();
    root["rssi"] = WiFi.RSSI();
    root["ssid"] = WiFi.SSID();
    root["bssid"] = WiFi.BSSIDstr();
    root["channel"] = WiFi.channel();
    root["subnet_mask"] = WiFi.subnetMask().toString();
    root["gateway_ip"] = WiFi.gatewayIP().toString();
    IPAddress dnsIP1 = WiFi.dnsIP(0);
    IPAddress dnsIP2 = WiFi.dnsIP(1);
    if (isIPSet(dnsIP1)) {
      root["dns_ip_1"] = dnsIP1.toString();
    }
    if (isIPSet(dnsIP2)) {
      root["dns_ip_2"] = dnsIP2.toString();
    }
  }
  response->setLength();
  request->send(response);
}
