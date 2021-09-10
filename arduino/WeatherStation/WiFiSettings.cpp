#include "WiFiSettings.h"
#include "IPUtils.h"

const char *StringSSID PROGMEM = "ssid";
const char *StringPassword PROGMEM = "password";

void printWifiSettings(String prefix, WiFiSettings *s) {
    Serial.print(prefix);
    Serial.print(F(" ssid: "));Serial.print(s->ssid);
    //Serial.print(F(", password: "));Serial.print(s->password);
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
    root[FPSTR(StringSSID)] = ssid;
    root[FPSTR(StringPassword)] = password;
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
  ssid = root[FPSTR(StringSSID)].as<const char *>();
  password = root[FPSTR(StringPassword)].as<const char *>();
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


WiFiSettingsEndpoint::WiFiSettingsEndpoint(AsyncWebServer* pServer,FSPersistence *pPersistence, Settings *pSettings):
    SettingsEndpoint(pServer, WIFI_SETTINGS_ENDPOINT_PATH, pPersistence, pSettings, 
    [](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      //WiFiSettings *wifiSettings = (WiFiSettings *)pSettings;
      // if(wifiSettings->password.length()>4) {
      //   jsonObject[FPSTR(StringPassword)] = obfuscateToken((wifiSettings->password);
      // }
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      WiFiSettings *wifiSettings = (WiFiSettings *)pSettings;
      const char *ssid = jsonObject[FPSTR(StringSSID)].as<const char *>();
      wifiSettings->setFilePath(String(F(WIFI_CONFIG_DIRECTORY "/")) + ssid);
      // const char *pass = jsonObject[FPSTR(StringPassword)].as<const char *>();
      // if(strchr(pass,'*')) {
      //   jsonObject[FPSTR(StringPassword)] = wifiSettings->password;
      // }
    }) {}

