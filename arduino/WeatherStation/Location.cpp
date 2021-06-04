#include <Arduino.h>
#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonStreamingParser.h>
#include <JsonListener.h>

class IPListener: public JsonListener {
  String _key;
  public:
    String city;
    String country;
    String utc_offset;
    String lang;
    float latitude;
    float longitude;
    virtual void whitespace(char c) {};
    virtual void startDocument() {};
    virtual void key(String key) {_key = key;};
    virtual void value(String value);
    virtual void endArray() {};
    virtual void endObject() {};
    virtual void startArray() {};
    virtual void startObject() {};
    virtual void endDocument() {};
} ipListener;


void IPListener::value(String value) {
  Serial.println("key: " + _key + " value: " + value);
  if ( _key == "ip")
    Serial.println( "External IP: " + value);
  if ( _key == "city")
    city = value;
  if ( _key == "country")
    country = value;
  if ( _key == "utc_offset")
    utc_offset = value;
  if ( _key == "languages")
    lang = value.substring(0, 2);
  if ( _key == "latitude")
    latitude = value.toFloat();
  if ( _key == "longitude")
    longitude = value.toFloat();
}

const char* const Countries_12h[] = { "EG", "BD", "IN", "JO", "PK", "PH", "MY", "SA", "US", "SV", "HN", "NI", "IE", "CA", "MX", "AU", "NZ", "CO"};
const char* const Countries_Fahrenheit[] = { "US", "BZ", "PW", "BS", "KY"};
const char* const Countries_DateYMD[] = { "BT", "CN", "HU", "JP", "KP", "KR", "LT", "MN", "TW", "US"};

void detectLocationFromIP( bool firstStart, String& location, int& utc_offset, char* lang, bool& b24h, bool& bYMD, bool& metric, float& latitude, float& longitude) {
  BearSSL::WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();  //Ignore certificate
  JsonStreamingParser parser;
  parser.setListener(&ipListener);

  http.begin(client, "https://ipapi.co/json");
  http.addHeader(F("Accept"), F("application/json"));
  int httpCode = http.GET();
  Serial.printf("Detect IP code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    int c = 1;
    while (http.connected() && c) {
      uint8_t payload;
      c = client.read(&payload, sizeof(payload));
      parser.parse(payload);
    }
  }
  http.end();

  if (httpCode != HTTP_CODE_OK)
    return;

  //Process utc_offset
  int utc_offset_old = utc_offset;
  bool minus = ipListener.utc_offset.charAt(0) == '-';
  if (minus || (ipListener.utc_offset.charAt(0) == '+'))  //remove sign
    ipListener.utc_offset.remove(0,1);
  //minutes - the last two characters;
  utc_offset = ipListener.utc_offset.substring( ipListener.utc_offset.length() - 2).toInt() * 60;
  ipListener.utc_offset.remove(ipListener.utc_offset.length() - 2, 2);
  //hours
  utc_offset += ipListener.utc_offset.toInt() * 60 * 60;
  if (minus)
    utc_offset = -utc_offset;
  
  if (!firstStart && (utc_offset_old != utc_offset))  //if utc offset is changed during refresh
    Serial.println( "UTC offset changed from " + String(utc_offset_old) + " to " + String(utc_offset));

  //Return other detected location values
  String country;
  country = ipListener.country;
  country.toUpperCase();
  latitude = ipListener.latitude;
  longitude = ipListener.longitude;
  location = ipListener.city + "," + country;

  if ( ipListener.lang == "cs")    //replace cs->cz code for weather
    ipListener.lang = "cz";
  strncpy(lang,ipListener.lang.c_str(),2);
 
  //24-hours vs 12-hours clock detection
  b24h = true;
  for (unsigned int i = 0; i < sizeof(Countries_12h) / sizeof(Countries_12h[0]); i++) {
    if (country == Countries_12h[i]) {
      b24h = false;
      break;
    }
  }

  //Celsius vs Fahrenheit detection
  metric = true;
  for (unsigned int i = 0; i < sizeof(Countries_Fahrenheit) / sizeof(Countries_Fahrenheit[0]); i++) {
    if (country == Countries_Fahrenheit[i]) {
      metric = false;
      break;
    }
  }
  
  bYMD = false;
  for (unsigned int i = 0; i < sizeof(Countries_DateYMD) / sizeof(Countries_DateYMD[0]); i++) {
    if (country == Countries_DateYMD[i]) {
      bYMD = true;
      break;
    }
  }
}
