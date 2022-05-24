#include "WiFiSettings.h"
#include "IPUtils.h"

const char *StringSSID PROGMEM = "ssid";
const char *StringPassword PROGMEM = "password";
const char *DisallowedFSChars = "#%&{}\\/<>*?/$!`'\":@+|=";

void WiFiSettings::print(const __FlashStringHelper *title) {
    Serial.print(title);
    Serial.print(F(" ssid: "));Serial.print(ssid);
    Serial.print(F(", password: "));Serial.print(obfuscateToken(password,0));
    Serial.print(F(", hostname: "));Serial.print(hostname);
    Serial.print(F(", static_ip_config: "));Serial.print(staticIPConfig);
    Serial.print(F(", local_ip: "));Serial.print(localIP);
    Serial.print(F(", gateway_ip: "));Serial.print(gatewayIP);
    Serial.print(F(", subnet_mask: "));Serial.print(subnetMask);
    Serial.print(F(", dns_ip_1: "));Serial.print(dnsIP1);
    Serial.print(F(", dns_ip_2: "));Serial.print(dnsIP2);
    Serial.println();
}

char *createDefaultHostname() {
    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    auto len = 13+strlen_P(PSTR(WIFI_HOSTNAME_PREFIX))+1;
    char *hostname = new char[len];
    snprintf_P(hostname, len, PSTR(WIFI_HOSTNAME_PREFIX "%02x%02x"), mac[4], mac[5]);
    return hostname;
}

static String SSIDtoFileName(const String &ssid) {
  String ret = ssid;
  for(int i=0;i<ret.length();i++) {
    if(strchr(DisallowedFSChars, ret[i])) {
      ret[i] = '-';
    }
  }
  if(ret.length()>31) {
    ret = ret.substring(0,31);
  }
  return ret;
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
    print(F("Save WiFi settings"));
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
  print(F("Load WiFi settings"));
  return 1;
}

WiFiSettingsManager::WiFiSettingsManager(FSPersistence *pPersistence):
  _pFsp(pPersistence) {
    
}

bool WiFiSettingsManager::saveSettings(WiFiSettings *pSettings) {
  String filepath = filenameForSSID(pSettings->ssid);
  Serial.printf_P(PSTR("[WSM] saveSettings of %s to %s\n"), pSettings->ssid.c_str(), filepath.c_str());
  if(filepath.length() > 0) {
    pSettings->setFilePath(filepath);
    _pFsp->writeToFS(pSettings);
    return true;
  }
  return false;
}

bool WiFiSettingsManager::loadSettings(WiFiSettings *pSettings, const String &ssid) {
  String filepath = findConfigWithSSID(ssid);
  Serial.printf_P(PSTR("[WSM] loadSettings of %s from %s\n"), ssid.c_str(), filepath.c_str());
  if(filepath.length() > 0) {
    pSettings->setFilePath(filepath);
    _pFsp->readFromFS(pSettings);
    return true;
  }
  return false;
}

String WiFiSettingsManager::findConfigWithSSID(const String &ssid) {
  String filename = SSIDtoFileName(ssid);
  Serial.printf_P(PSTR("[WSM] findConfigWithSSID %s: %s\n"), ssid.c_str(), filename.c_str());
  WiFiSettings wifiSettings;
  // Check exact filename
  String filepath = F(WIFI_CONFIG_DIRECTORY "/") + filename;
  if(_pFsp->existsConfig(filepath)) {
    wifiSettings.setFilePath(filepath);
    Serial.printf_P(PSTR(" Opening %s\n"), filepath.c_str());
    _pFsp->readFromFS(&wifiSettings);
     Serial.printf_P(PSTR("    read %s\n"), wifiSettings.ssid.c_str());
    if(wifiSettings.ssid == ssid) {
      return filepath;
    }
  }
  // find in files
  std::vector<String> list = _pFsp->listConfigs(F(WIFI_CONFIG_DIRECTORY));
  for(const String &file: list) {
    wifiSettings.setFilePath(file);
    Serial.printf_P(PSTR(" Opening %s\n"), wifiSettings.getFilePath().c_str());
    _pFsp->readFromFS(&wifiSettings);
     Serial.printf_P(PSTR("    read %s\n"), wifiSettings.ssid.c_str());
    if(wifiSettings.ssid == ssid) {
      return file;
    }
  };
  return "";
}

String WiFiSettingsManager::filenameForSSID(const String &ssid) {
  String filename = SSIDtoFileName(ssid);
  Serial.printf_P(PSTR("[WSM] filenameForSSID %s: %s\n"), ssid.c_str(), filename.c_str());
  WiFiSettings wifiSettings;
  // Check exact filename
  int i=0;
  int j=1;
  String basename = filename;
  if(filename.length()>29) {
    basename = filename.substring(0,29);
  }
  String filepath = F(WIFI_CONFIG_DIRECTORY "/") + filename;
  while(_pFsp->existsConfig(filepath)) {
    wifiSettings.setFilePath(filepath);
    Serial.printf_P(PSTR(" Opening %s\n"), filepath.c_str());
    _pFsp->readFromFS(&wifiSettings);
    Serial.printf_P(PSTR("    read %s\n"), wifiSettings.ssid.c_str());
    if(wifiSettings.ssid == ssid) {
      return filepath;
    }
    filename = basename;
    filename.concat('~');
    filename.concat(i);
    filepath = F(WIFI_CONFIG_DIRECTORY "/") + filename;
    Serial.printf_P(PSTR("  trying %s\n"), filepath.c_str());
    ++i;
    if(i==10) {
      ++j;
      if(basename.length()>(30-j)) {
        basename = basename.substring(0,30-j);
      }
    }
  }
  return filepath;
}

std::vector<String> WiFiSettingsManager::listSavedWiFiNetworks() {
  std::vector<String> list;
  WiFiSettings wifiSettings;
  Serial.println(F("[WSM] listSavedWiFiNetworks"));
  _pFsp->traverseConfigs([&](const String &path, const String &filename){
    wifiSettings.setFilePath(path + "/" + filename);
    Serial.printf_P(PSTR(" Opening %s\n"), wifiSettings.getFilePath().c_str());
    _pFsp->readFromFS(&wifiSettings);
    Serial.printf_P(PSTR("    read %s\n"), wifiSettings.ssid.c_str());
    list.push_back(wifiSettings.ssid);
  }, F(WIFI_CONFIG_DIRECTORY));
  return list;
}

int WiFiSettingsManager::savedWiFiNetworksCount() {
   int count = 0;
  _pFsp->traverseConfigs([&](const String &, const String &){
    ++count;
  }, F(WIFI_CONFIG_DIRECTORY));
  Serial.printf_P(PSTR("[WSM] savedWiFiNetworksCount found %d\n"),count);
  return count;
}

void WiFiSettingsManager::removeSettings(const String &ssid) {
  String filename = findConfigWithSSID(ssid);
  Serial.printf_P(PSTR("[WSM] removeSettings of %s: file %s\n"), ssid.c_str(), filename.c_str());
  if(filename.length()>0) {
    _pFsp->removeConfig(filename);
  }
}


WiFiSettingsEndpoint::WiFiSettingsEndpoint(FSPersistence *pPersistence, Settings *pSettings):
    SettingsEndpoint(WIFI_SETTINGS_ENDPOINT_PATH, pPersistence, pSettings,
    [](Settings *pSettings, JsonObject jsonObject) { //fetchManipulator
      WiFiSettings *wifiSettings = static_cast<WiFiSettings *>(pSettings);
      jsonObject[FPSTR(StringPassword)] = obfuscateToken(wifiSettings->password, 0);
    },[](Settings *pSettings, JsonObject jsonObject) { //updateManipulator
      WiFiSettings *wifiSettings = static_cast<WiFiSettings *>(pSettings);
      const char *pass = jsonObject[FPSTR(StringPassword)].as<const char *>();
      if(!strcmp(pass, ReplaceMark)) {
        jsonObject[FPSTR(StringPassword)] = wifiSettings->password;
      }
    }, false) {}

