#pragma once
#include <Arduino.h>

String strTime(time_t timestamp, bool shortTime);
String strTimeSuffix(time_t timestamp);
String strDate(time_t timestamp, bool shortDate);
String strTempUnit();
String strTemp( float t);
String strHum( float h);
String strWind( float w);
String utf8ascii(const String s);

struct tConfig {
  bool detectLocationIP;
  unsigned int updateDataMin;
  String openweatherApiKey;
// Go to https://openweathermap.org/find?q= and search for a location  
  String location;
  char language[3];
  int utcOffset;  //in seconds
  float latitude;
  float longitude;
  bool useMetric;
  bool use24hour;
  bool useYMDdate;
  String ntp;
  int8_t tempOffset;
  int8_t humOffset;
  String iotCenterUrl;
  unsigned int iotRefreshMin;
};

struct tForecast {
  uint32_t observationTime;
  int temp;
  char iconMeteoCon;
  unsigned int windSpeed;
  unsigned int windDeg;
};

struct tCurrentWeather {
  int temp;
  int tempMin;
  int tempMax;
  String description;
  unsigned int windSpeed;
  char iconMeteoCon;
  uint32_t sunrise;
  uint32_t sunset;
};

extern tConfig conf;

void setLanguage( const char* lang);
String getDayName( uint8_t index);
String getMonthName( uint8_t index);
String getMoonPhaseName( uint8_t index);

//List of all strings
enum tStrings {
  s_Connecting_WiFi=0, s_Connecting_IoT_Center, s_Detecting_location, s_Updating_time, s_Updating_weather, s_Calculate_moon_phase, s_Updating_forecasts, s_Connecting_InfluxDB, s_Done,
  s_In, s_Out,
  s_INDOOR, s_feel, s_hum, s_wind,
  s_Moon, s_Sun
};
String getStr( uint8_t index);
