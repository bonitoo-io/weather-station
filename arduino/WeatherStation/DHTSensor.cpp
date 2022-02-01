#include <OLEDDisplayUi.h>
#include <DHT.h>
#include "DHTSensor.h"
#include "Tools.h"
#include "WeatherStation.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

// Internal sensor settings
#define DHT_TYPE DHT11  // Sensor DHT 11
#define PIN_DHT D1      // Digital pin connected to the DHT 11 sensor - GPIO5

DHT dht(PIN_DHT, DHT_TYPE);
int16_t tempHistory[90];

void setupDHT() {
  dht.begin();
  //clean data
  for (int i = 0; i < sizeof(tempHistory) / sizeof(tempHistory[0]); i++)
    tempHistory[i] = NO_VALUE;
  float h = dht.readHumidity();
  
  //fix humidity offset overflow
  if (station.getAdvancedSettings()->humOffset + h > 100) { 
    station.getAdvancedSettings()->humOffset = 0;
    Serial.println( F("DHT humidity overflow, zeroing offset"));
  }

  //autocalibrate sensors with faulty humidity
  if (( h < 36) && (station.getAdvancedSettings()->humOffset == 0)) { 
    station.getAdvancedSettings()->humOffset = 55 - h;
    Serial.print( F("DHT humidity autocalibration offset: "));
    Serial.println( station.getAdvancedSettings()->humOffset);
  }
}

float lastTemp = NAN;
float lastHum = NAN;

void refreshDHTCachedValues(bool metric) {
  getDHTTemp( metric);
  getDHTHum();
  //Serial.println( "refreshDHTCachedValues " +  String(lastTemp) + " " + String(lastHum));
}

float getDHTCachedTemp() {
  return lastTemp;
}

float getDHTCachedHum() {
  return lastHum;
}

float getDHTTemp(bool metric) {
  float t = dht.readTemperature(!metric);
  if (isnan(t)) {
    Serial.println( F("Received NAN temperature!"));
    return lastTemp;
  }
  t += station.getAdvancedSettings()->tempOffset;
  
  if (isnan( lastTemp) || (abs(t - lastTemp) < 10))   //skip potentially wrong values
    lastTemp = t;
  else {
    Serial.print( F("Received wrong temperature: new/last"));
    Serial.println( String( t) + " / " + String(lastTemp));
  }
  return lastTemp;
}

float getDHTHum() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println( F("Received NAN humidity!"));
    return lastHum;
  }
  h += station.getAdvancedSettings()->humOffset;
  if (h > 100)
    h = 100;
  if (h < 0)
    h = 0;
  if (isnan( lastHum) || (abs(h - lastHum) < 10))   //skip potentially wrong values
    lastHum = h;
  else {
    Serial.print( F("Received wrong humidity: new/last"));
    Serial.println( String( h) + " / " + String(lastHum));
  }
  return lastHum;
}

float getDHTHic(bool metric) {
  float tempDHT = getDHTTemp( metric);
  float humDHT = getDHTHum();
  return isnan(tempDHT) ? NAN : dht.computeHeatIndex(tempDHT, humDHT, !metric);
}

void saveDHTTempHist(bool metric) {
  float t = getDHTTemp(metric);
  if (isnan(t))
    return;
  
  for (uint8_t i = 0; i < (sizeof(tempHistory) / sizeof(tempHistory[0])) - 1; i++) //move all values left
    tempHistory[i] = tempHistory[i+1];
  tempHistory[(sizeof(tempHistory) / sizeof(tempHistory[0])) - 1] = round( t * 10); //save the latest value
}

void sensorError( OLEDDisplay *display, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString((display->getWidth() / 2) + x, 21 + y, getStr(s_Temperature_sensor_error));
}

void drawDHT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (isnan(getDHTTemp(station.getRegionalSettings()->useMetricUnits))) {
    sensorError( display, x ,y);
    return;
  }
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 6 + y, getStr(s_INDOOR));

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 + x, 36 + y, getStr(s_feel) + strTemp(getDHTHic(station.getRegionalSettings()->useMetricUnits)));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(120 + x, 36 + y, getStr(s_hum));

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  
  display->drawString(8 + x, 15 + y, strTemp(getDHTTemp(station.getRegionalSettings()->useMetricUnits)));
  display->drawString(80 + x, 15 + y, strHum(getDHTHum()));

  display->setFont(Meteocons_Plain_21);
  display->drawString(-7 + x, 19 + y, F("'")); //show thermomether symbol
}
