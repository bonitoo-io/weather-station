#pragma once
#include <Arduino.h>

String strTime(time_t timestamp, bool shortTime);
String strTimeSuffix(time_t timestamp);
String strDate(time_t timestamp, bool shortDate);
String strWind( unsigned int w);
String utf8ascii(const String s);
const char *getDeviceID();

struct tConfig {
//  unsigned int updateDataMin;
//  String openweatherApiKey;
//  String ntp;
//  int8_t tempOffset;
//  int8_t humOffset;
  String iotCenterUrl;
  unsigned int iotRefreshMin;
};

struct tForecast {
  uint32_t observationTime;
  int16_t temp;
  char iconMeteoCon;
  unsigned int windSpeed;
  unsigned int windDeg;
};

struct tCurrentWeather {
  int16_t temp;
  int16_t tempMin;
  int16_t tempMax;
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
  s_Connecting_WiFi=0, s_Searching, s_or_wait_for_setup, s_Connecting_IoT_Center, s_Reset_wait, s_Detecting_location, s_Checking_update, s_Updating_time, s_Updating_weather, s_Calculate_moon_phase, s_Updating_forecasts, s_Connecting_InfluxDB, s_Done,
  s_Update_found, s_Update_start_in, s_Update_successful, s_Update_restart_in, s_Update_failed, s_Updating_to, s_Update_restarting, s_Wifi_AP_connect, s_Wifi_web_point, s_Wifi_configure,
  s_InfluxData_Weather_Station, s_Configure_via, s_Forecast_error,
  s_In, s_Out, s_now, s_Temperature_sensor_error,
  s_INDOOR, s_feel, s_hum, s_wind,
  s_Moon, s_Sun,
  s_indoor_spread_risk, s_Low, s_Medium, s_High,
};

String getStr( uint8_t index);
