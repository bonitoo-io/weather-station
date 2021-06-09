#ifndef IP_UTILS_H
#define IP_UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>

const IPAddress IP_NOT_SET = IPAddress(INADDR_NONE);

inline bool isIPSet(const IPAddress& ip) {
    return ip != IP_NOT_SET;
};

inline void readIP(JsonObject& root, const String& key, IPAddress& ip, const IPAddress& defaultIp = INADDR_NONE) {
    if (!root[key].is<String>() || !ip.fromString(root[key].as<String>())) {
        ip = defaultIp;
    }
};

inline void writeIP(JsonObject& root, const String& key, const IPAddress& ip) {
    if (isIPSet(ip)) {
        root[key] = ip.toString();
    }
};

inline IPAddress ipFromString(const char *strIp) {
   IPAddress ip;
   ip.fromString(strIp);
   return ip;
};

#endif // IP_UTILS_H
