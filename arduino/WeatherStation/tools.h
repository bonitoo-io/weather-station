#pragma once
#include <Arduino.h>

// Adjust according to your language
extern const char* const WDAY_NAMES[];
extern const char* const MONTH_NAMES[];
extern const char* const MOON_PHASES[];

String strTime(time_t timestamp, bool shortTime);
String strDate(time_t timestamp, bool shortDate);
String utf8ascii(const String s);

struct tConfig {
  String wifi_ssid;
  String wifi_pwd;

  bool detectLocationIP;
  int update_data_min;
  String openweatherApiKey;
// Go to https://openweathermap.org/find?q= and search for a location  
  String location;
  String language;
  int utcOffset;
  float latitude;
  float longitude;
  bool useMetric;
  bool use24hour;
  String ntp;

  String influxdbUrl;
  String influxdbToken;
  String influxdbOrg;
  String influxdbBucket;
  int influxdbRefreshMin;
};
