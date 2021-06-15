#include <OLEDDisplayUi.h>
#include <DHT.h>
#include "Tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

// Internal sensor settings
#define DHTTYPE DHT11   // DHT 11
#define DHTPIN D1       // Digital pin connected to the DHT sensor

DHT dht(DHTPIN, DHTTYPE);
int tempHistory[90];

void setupDHT() {
  dht.begin();
  //clean data
  for (int i = 0; i < sizeof(tempHistory) / sizeof(tempHistory[0]); i++)
    tempHistory[i] = 0xffff;
}

float getDHTTemp(bool metric) {
  return dht.readTemperature(!metric) + conf.tempOffset;
}

float getDHTHum() {
  return dht.readHumidity() + conf.humOffset;
}

float getDHTHic(bool metric) {
  float tempDHT = getDHTTemp( metric);
  float humDHT = getDHTHum();
  return dht.computeHeatIndex(tempDHT, humDHT, !metric);
}

void saveDHTTempHist() {
  //move all values left
  for (int i = 1; i < sizeof(tempHistory) / sizeof(tempHistory[0]); i++)
    tempHistory[i-1] = tempHistory[i];
  tempHistory[(sizeof(tempHistory) / sizeof(tempHistory[0])) - 1] = round( getDHTTemp(true) * 10); //save the latest value
}

void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 6 + y, getStr(s_INDOOR));

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 + x, 38 + y, getStr(s_feel) + strTemp(getDHTHic(conf.useMetric)));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(120 + x, 38 + y, getStr(s_hum));

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  display->drawString(8 + x, 15 + y, strTemp(getDHTTemp(conf.useMetric)));
  display->drawString(80 + x, 15 + y, strHum(getDHTHum()));

  display->setFont(Meteocons_Plain_21);
  display->drawString(-7 + x, 19 + y, F("'")); //show thermomether symbol
}
