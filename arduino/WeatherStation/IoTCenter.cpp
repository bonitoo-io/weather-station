#include <time.h>
#include <ESPHTTPClient.h>
#include "InfluxDBHelper.h"

//Load value for specific parameter
String loadParameter( const String& response, const __FlashStringHelper* param_p) {
  String param(FPSTR(param_p));
  int i = response.indexOf(param);
  if (i == -1) {
    Serial.print(F("Error - missing parameter: "));
    Serial.println( param);
    return "";
  }
  return response.substring( response.indexOf(":", i) + 2, response.indexOf("\n", i));
}

bool loadIoTCenter( bool firstStart, const String& iot_url, const String &deviceID, InfluxDBSettings *influxdbSettings, unsigned int& iotRefreshMin, float& latitude, float& longitude) {
  if ( iot_url.length() == 0) //if iot_center url is not defined, exit
    return false;
  
  // Load config from IoT Center
  WiFiClient client;
  HTTPClient http_config;
  String payload;
  String url = iot_url + String(F("/api/env/")) + deviceID;
  Serial.print(F("Connecting "));
  Serial.println( url);
  http_config.begin( client, url);
  http_config.addHeader(String(F("Accept")), String(F("text/plain")));
  int httpCode = http_config.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    payload = http_config.getString();
    Serial.println(F("--Received configuration"));
    Serial.print(payload);
    Serial.println(F("--end"));
  } else {
    Serial.print(F("IoT GET failed, error: "));
    Serial.println( http_config.errorToString(httpCode).c_str());
  }
  http_config.end();

  //Parse response, if exists
  if ( payload.length() > 0) {
    //Sync time from IoT Center
    String iotTime = loadParameter( payload, F("serverTime"));
    /*tm tmServer;
    strptime(iotTime.c_str(), "%Y-%m-%dT%H:%M:%S.%f", &tmServer);
    time_t ttServer = mktime(&tmServer);
    struct timeval tvServer = { .tv_sec = ttServer };
    settimeofday(&tvServer, NULL);

    // Show time
    ttServer = time(nullptr);
    Serial.print(F("Set time: "));
    Serial.print(String(ctime(&ttServer)));*/

    //Load InfluxDB parameters
    influxdbSettings->serverURL = loadParameter( payload, F("influx_url"));
    influxdbSettings->org = loadParameter( payload, F("influx_org"));
    influxdbSettings->authorizationToken = loadParameter( payload, F("influx_token"));
    influxdbSettings->bucket = loadParameter( payload, F("influx_bucket"));

    //Load refresh parameters
    influxdbSettings->writeInterval = loadParameter( payload, F("measurement_interval")).toInt();
    if (influxdbSettings->writeInterval == 0)
      influxdbSettings->writeInterval  = 1;

    iotRefreshMin = loadParameter( payload, F("configuration_refresh")).toInt();
    if (iotRefreshMin == 0)
      iotRefreshMin = 60;
    
    latitude = loadParameter( payload, F("default_lat")).toDouble();
    longitude = loadParameter( payload, F("default_lon")).toDouble();
  } else {
    Serial.println(F("IoT Center GET failed, emty response"));
    return false;
  }
  return true;
}
