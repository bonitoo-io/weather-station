#include <OLEDDisplayUi.h>
#include <DHT.h>
#include "tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

extern tConfig conf;

// Internal sensor settings
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN D1       // Digital pin connected to the DHT sensor

DHT dht(DHTPIN, DHTTYPE);
float tempDHT;
float humDHT;
float hicDHT;
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
    hicDHT = dht.computeHeatIndex(tempDHT, humDHT, !metric);
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
  display->drawString(8 + x, 38 + y, "feel: " + strTemp(hicDHT));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(120 + x, 38 + y, "hum");

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  display->drawString(8 + x, 15 + y, strTemp(getDHTTemp(conf.useMetric)));
  display->drawString(80 + x, 15 + y, strHum(getDHTHum()));

  display->setFont(Meteocons_Plain_21);
  display->drawString(-7 + x, 19 + y, "'"); //show thermomether symbol
}
