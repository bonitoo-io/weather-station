#include <Arduino.h>
#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonStreamingParser.h>
#include <JsonListener.h>

class tIPListener: public JsonListener {
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
};


void tIPListener::value(String value) {
  //Serial.println(String(F("key: ")) + _key + String(F(" value: ")) + value);
  if ( _key == "ip") {
    Serial.print( F("External IP: "));
    Serial.println( value);
  }
  if ( _key == String(F("city")))
    city = value;
  if ( _key == String(F("country")))
    country = value;
  if ( _key == String(F("utc_offset")))
    utc_offset = value;
  if ( _key == String(F("languages")))
    lang = value.substring(0, 2);
  if ( _key == String(F("latitude")))
    latitude = value.toFloat();
  if ( _key == String(F("longitude")))
    longitude = value.toFloat();
}

const char Countries12h[] PROGMEM = "EG" "BD" "IN" "JO" "PK" "PH" "MY" "SA" "US" "SV" "HN" "NI" "IE" "CA" "MX" "AU" "NZ" "CO";
const char CountriesFahrenheit[] PROGMEM = "US" "BZ" "PW" "BS" "KY";
const char CountriesDateYMD[] PROGMEM = "BT" "CN" "HU" "JP" "KP" "KR" "LT" "MN" "TW" "US";


bool findCountry( const char* country, const char* list) {
  uint16_t cw = (country[1] << 8) + country[0];
  const char* pl = list;
  while ( pgm_read_byte( pl) != 0) {  //end of list?
    if ( cw == pgm_read_word( pl))
      return true;
    pl += 2;   
  }
  return false;
}

void detectLocationFromIP( bool firstStart, String& location, int& utc_offset, char* lang, bool& b24h, bool& bYMD, bool& metric, float& latitude, float& longitude) {
  BearSSL::WiFiClientSecure client;
  HTTPClient http;
  tIPListener ipListener;

  client.setInsecure();  //Ignore certificate
  JsonStreamingParser parser;
  parser.setListener(&ipListener);

  http.begin(client, String(F("https://ipapi.co/json")));
  http.addHeader(F("Accept"), F("application/json"));
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    int c = 1;
    while (http.connected() && c) {
      uint8_t payload;
      c = client.read(&payload, sizeof(payload));
      parser.parse(payload);
    }
  } else {
    Serial.print( F("Detect IP error code: "));
    Serial.println( httpCode);
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
  
  if (!firstStart && (utc_offset_old != utc_offset)) { //if utc offset is changed during refresh
    Serial.print( F("UTC offset changed from "));
    Serial.println( String(utc_offset_old) + String(F( " to ")) + String(utc_offset));
  }

  //Return other detected location values
  String country;
  country = ipListener.country;
  country.toUpperCase();
  latitude = ipListener.latitude;
  longitude = ipListener.longitude;
  location = ipListener.city + "," + country;

  if ( ipListener.lang == "cs")    //replace cs->cz code for weather
    ipListener.lang = "cz";
  strncpy( lang, ipListener.lang.c_str(), 2);
  
  //24-hours vs 12-hours clock detection
  b24h = !findCountry(country.c_str(), Countries12h);

  //Celsius vs Fahrenheit detection
  metric = !findCountry(country.c_str(), CountriesFahrenheit);
  
  //Date format YMD vs DMY
  bYMD = findCountry(country.c_str(), CountriesDateYMD);
}
