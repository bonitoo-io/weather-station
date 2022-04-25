#include <Arduino.h>
#include <ESPWiFi.h>
#include <ESPHTTPClient.h>
#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include "RegionalSettings.h"

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
const char nonLatinLang2Eng[] PROGMEM = "ar" "th" "zh" "he" "ja" "ko";

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

// Returns 0 in case of failure, 1- ok, city or UTC offset not changed, 2 - ok, city or UTC offset changed
int detectLocationFromIP(RegionalSettings *pRegionalSettings) {
  BearSSL::WiFiClientSecure client;
  HTTPClient http;
  tIPListener ipListener;

  client.setInsecure();  //Ignore certificate
  JsonStreamingParser parser;
  parser.setListener(&ipListener);
  Serial.println(F("Getting IP based regional info"));

  http.begin(client, String(F("https://ipapi.co/json")));
  http.addHeader(F("Accept"), F("application/json"));
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    Serial.println();
    int c = 1;
    while (http.connected() && c) {
      uint8_t payload;
      c = client.read(&payload, sizeof(payload));
      parser.parse(payload);
    }

  } else {
    Serial.print(F(" error: "));
    Serial.println( httpCode);
  }
  http.end();
  if (httpCode != HTTP_CODE_OK) {
    return 0;
  }

  //Process utc_offset
  bool minus = ipListener.utc_offset.charAt(0) == '-';
  if (minus || (ipListener.utc_offset.charAt(0) == '+'))  //remove sign
    ipListener.utc_offset.remove(0,1);
  //minutes - the last two characters;
  int utc_offset = ipListener.utc_offset.substring( ipListener.utc_offset.length() - 2).toInt() * 60;
  ipListener.utc_offset.remove(ipListener.utc_offset.length() - 2, 2);
  //hours
  utc_offset += ipListener.utc_offset.toInt() * 60 * 60;
  if (minus)
    utc_offset = -utc_offset;

  bool changed = false;
  if (pRegionalSettings->utcOffset != utc_offset) { //if utc offset is changed during refresh
    Serial.print( F("UTC offset changed from "));
    Serial.println( String(pRegionalSettings->utcOffset) + String(F( " to ")) + String(utc_offset));
    pRegionalSettings->utcOffset = utc_offset;
    changed = true;
  }

  //Return other detected location values
  String country = ipListener.country;
  country.toUpperCase();
  String location = ipListener.city + "," + country;
  if (pRegionalSettings->location != location) {
    Serial.print( F("Location changed from "));
    Serial.print(pRegionalSettings->location);
    Serial.print( F(" to "));
    Serial.println(location);

    pRegionalSettings->location = location;
    pRegionalSettings->latitude = ipListener.latitude;
    pRegionalSettings->longitude = ipListener.longitude;
    pRegionalSettings->utcOffset = utc_offset;
    changed = true;
  }

  //interface language
  if ( ipListener.lang == "cs")    //replace cs->cz code for weather
    ipListener.lang = "cz";
  if ( pRegionalSettings->language != ipListener.lang) {
    pRegionalSettings->language = ipListener.lang;
    changed = true;
  }

  //24-hours vs 12-hours clock detection
  bool use24Hours = !findCountry(country.c_str(), Countries12h);
  if ( pRegionalSettings->use24Hours != use24Hours) {
    pRegionalSettings->use24Hours = use24Hours;
    changed = true;
  }

  //Celsius vs Fahrenheit detection
  bool useMetricUnits = !findCountry(country.c_str(), CountriesFahrenheit);
  if ( pRegionalSettings->useMetricUnits != useMetricUnits) {
    pRegionalSettings->useMetricUnits = useMetricUnits;
    changed = true;
  }

  //Date format YMD vs DMY
  bool useYMDFormat = findCountry(country.c_str(), CountriesDateYMD);
  if ( pRegionalSettings->useYMDFormat != useYMDFormat) {
    pRegionalSettings->useYMDFormat = useYMDFormat;
    changed = true;
  }

  return changed?2:1;
}


bool nonLatin2Eng( const char* lang) {
  return findCountry(lang, nonLatinLang2Eng);
}