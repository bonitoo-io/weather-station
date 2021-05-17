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
    int utc_offset = 0;
  
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
  //Serial.println("key: " +_key + " value: " + value);
  if ( _key == "city")
    city = value; 
  if ( _key == "country")
    country = value; 
  if ( _key == "utc_offset")
    utc_offset = value.toInt(); 
}

void detectLocationFromIP( String& location, int& utc_offset) {
  BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure;
  HTTPClient http;
 
  client->setInsecure();  //Ignore certificate
  JsonStreamingParser parser;
  parser.setListener(&ipListener);
  
  http.begin(*client, "https://ipapi.co/json");
  http.addHeader(F("Accept"), F("application/json"));
  int httpCode = http.GET();
  Serial.printf("Detect IP code: %d\n", httpCode);
  
  if (httpCode == HTTP_CODE_OK) {
    int c = 1; 
    while (http.connected() && c) {
      uint8_t payload;
      c = client->read(&payload, sizeof(payload));
      parser.parse(payload);
    }
  }
  http.end();
  location = ipListener.city + "," + ipListener.country;
  utc_offset = ipListener.utc_offset;
}



/*
 * {
  "ip": "213.220.204.235",
  "version": "IPv4",
  "city": "Prague",
  "region": "Hlavni mesto Praha",
  "region_code": "10",
  "country": "CZ",
  "country_name": "Czechia",
  "country_code": "CZ",
  "country_code_iso3": "CZE",
  "country_capital": "Prague",
  "country_tld": ".cz",
  "continent_code": "EU",
  "in_eu": true,
  "postal": "130 00",
  "latitude": 50.0804,
  "longitude": 14.5045,
  "timezone": "Europe/Prague",
  "utc_offset": "+0200",
  "country_calling_code": "+420",
  "currency": "CZK",
  "currency_name": "Koruna",
  "languages": "cs,sk",
  "country_area": 78866.0,
  "country_population": 10625695.0,
  "asn": "AS16019",
  "org": "Vodafone Czech Republic a.s."
}

{
  "ip": "91.103.160.29",
  "version": "IPv4",
  "city": "Olomouc",
  "region": "Olomoucky kraj",
  "region_code": "71",
  "country": "CZ",
  "country_name": "Czechia",
  "country_code": "CZ",
  "country_code_iso3": "CZE",
  "country_capital": "Prague",
  "country_tld": ".cz",
  "continent_code": "EU",
  "in_eu": true,
  "postal": "783 16",
  "latitude": 49.6474,
  "longitude": 17.3285,
  "timezone": "Europe/Prague",
  "utc_offset": "+0200",
  "country_calling_code": "+420",
  "currency": "CZK",
  "currency_name": "Koruna",
  "languages": "cs,sk",
  "country_area": 78866.0,
  "country_population": 10625695.0,
  "asn": "AS42000",
  "org": "Futruy s.r.o."
}
 */
