#pragma once
#include <Arduino.h>

String strTime(time_t timestamp, bool shortTime);
String strTimeSuffix(time_t timestamp);
String strDate(time_t timestamp, bool shortDate);
String strTemp( float t);
String strHum( float h);
String strWind( float w);
String utf8ascii(const String s);

struct tConfig {
  String wifi_ssid;
  String wifi_pwd;

  bool detectLocationIP;
  unsigned int updateDataMin;
  String openweatherApiKey;
// Go to https://openweathermap.org/find?q= and search for a location  
  String location;
  String language;
  int utcOffset;
  float latitude;
  float longitude;
  bool useMetric;
  bool use24hour;
  bool useYMDdate;
  String ntp;
  int8_t tempOffset;
  int8_t humOffset;

  String influxdbUrl;
  String influxdbToken;
  String influxdbOrg;
  String influxdbBucket;
  unsigned int influxdbRefreshMin;
};

struct tForecast {
  uint32_t observationTime;
  int temp;
  char iconMeteoCon;
  int windSpeed;
  int windDeg;
};

struct tCurrentWeather {
  int temp;
  int tempMin;
  int tempMax;
  String description;
  int windSpeed;
  char iconMeteoCon;
  uint32_t sunrise;
  uint32_t sunset;
};

extern tConfig conf;

// Adjust according to your language
extern const char* const WDAY_NAMES[];
extern const char* const MONTH_NAMES[];
extern const char* const MOON_PHASES[];
