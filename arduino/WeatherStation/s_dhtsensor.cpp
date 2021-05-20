#include <OLEDDisplayUi.h>
#include <DHT.h>
#include "tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

extern bool g_bMetric;

// Internal sensor settings
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN D1       // Digital pin connected to the DHT sensor


DHT dht(DHTPIN, DHTTYPE);
float tempDHT;
float humDHT;
unsigned long timeDHT = 0;


void setupDHT() {
  dht.begin();  
}

void _readDHT( bool metric) {
  unsigned long actualTime = millis();
  if ( timeDHT > actualTime)
    timeDHT = actualTime;
  if ( actualTime - timeDHT > 2000) { //read once 2 seconds, otherwise provide "cached" values
    tempDHT = dht.readTemperature(!metric);
    humDHT = dht.readHumidity();
    timeDHT = millis();
  }
}

float getDHTTemp(bool metric) {
  _readDHT(metric);
  return tempDHT;
}

float getDHTHum() {
  return humDHT;
}
  

void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 5 + y, "INDOOR");

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 10 + y, "Temp");
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(120 + x, 10 + y, "Hum");

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  display->drawString(0 + x, 20 + y, String(getDHTTemp(g_bMetric), 1) + (g_bMetric ? "°C" : "°F"));
  display->drawString(80 + x, 20 + y, String(getDHTHum(), 0) + "%");
}
