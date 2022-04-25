#include <OLEDDisplayUi.h>
#include "WeatherStationFonts.h"
#include "tools.h"
#include "WeatherStation.h"
#include "Sensor.h"
#include "SensorDHT.h"
#include "SensorSHT.h"

#define DEC_PLACES 0

Sensor* pSensor = nullptr;

bool setupSensor() {
  if (SensorSHT::driverDetect()) {
    Serial.println( F("Detected sensor SHTC3"));
    pSensor = new SensorSHT();
  } else {
    Serial.println( F("Using sensor DHT11"));
    pSensor = new SensorDHT();
  }
  if (pSensor) {
    if (pSensor->setup())  //initialize sensor
      return true;
    else
      Serial.println( F("Sensor setup error!"));
  }
  return false;
}

bool Sensor::setup() {
  driverSetup(); //initialize sensor
  float t = driverGetTempF();
  _timeNextUpdate = millis() + driverGetMaxRefreshRateMs();
  Serial.println( String(F("Temp raw = ")) + String(t) + F(" ") + String(tempF2C(t)));
  float h = driverGetHum(true);
  Serial.println( String(F("Hum raw = ")) + String(h));
  //fix humidity offset overflow
  if ((station.getAdvancedSettings()->getHumOffset() != 0) && (station.getAdvancedSettings()->getHumOffset() + h > 100)) {
    station.getAdvancedSettings()->setHumOffset(0);
    Serial.println( F("Sensor humidity overflow, zeroing offset"));
  }
  t += station.getAdvancedSettings()->getTempOffsetF();
  h += station.getAdvancedSettings()->getHumOffset();

  _tempFilt.init( t); //prepare median filter data for temperature
  _humFilt.init( float2Int( h)); //prepare median filter data for humidity
  //clean data
  for (uint8_t i = 0; i < TEMP_HIST_SIZE; i++)
    _tempHistory[i] = NO_VALUE_INT;
  return true;
}

float Sensor::getTempF( bool forceCached) {
  if (forceCached || (_timeNextUpdate >= millis()))
    return _tempFilt.getValue();

  float t = driverGetTempF(); //read temperature from the sensor
  _timeNextUpdate = millis() + driverGetMaxRefreshRateMs();    //next time to read metrics
  //Serial.println( "Temperature = " + String(t) + "->" + String(tempF2C(t)));
  if (isnan(t)) {
    Serial.println( F("Received NAN temperature!"));
    return _tempFilt.getValue();  //restore old value
  }
  internalLoadHum( true); //also process humidity

  t += station.getAdvancedSettings()->getTempOffsetF(); //Add offset in fahrenheit

  return _tempFilt.medianFilter(t);
}

void Sensor::internalLoadHum( bool secondRead) {
  float h = driverGetHum( secondRead); //read humidity from the sensor
  _timeNextUpdate = millis() + driverGetMaxRefreshRateMs();    //next time to read metrics
  //Serial.println( "Humidity = " + String(h));
  if (isnan(h)) {
    Serial.println( F("Received NAN humidity!"));
    return;
  }
  h += station.getAdvancedSettings()->getHumOffset(); //Add offset
  if (h > 100) { //fix boundaries
    h = 100;
  } else if (h < 0) {
    h = 0;
  }
  _humFilt.medianFilter(float2Int(h));
}

float Sensor::getHum( bool forceCached) {
  if (!(forceCached || (_timeNextUpdate >= millis())))
    internalLoadHum( false);
  return int2Float( _humFilt.getValue());
}

int16_t Sensor::getHist( uint8_t pos) {
  if ( pos < TEMP_HIST_SIZE)
    return station.getRegionalSettings()->useMetricUnits ? float2Int(tempF2C(int2Float(_tempHistory[pos]))) : _tempHistory[pos];
  return NO_VALUE_INT;
}

int16_t Sensor::temp2Int( float temp, bool metric) {
  if (isnan(temp))
    return NO_VALUE_INT;
  return float2Int( metric ? tempC2F( temp) : temp);
}

String Sensor::strTempUnit() {
  return String(station.getRegionalSettings()->useMetricUnits ? F("°C") : F("°F"));
}

String Sensor::strTempValue( float t, uint8_t decimalPlaces) {
  if (station.getRegionalSettings()->useMetricUnits)  //convert to C, if needed
    t = tempF2C(t);
  return (isnan(t) ? String(F("??")) : String(t,decimalPlaces));
}

String Sensor::strHum( float h, uint8_t decimalPlaces) {
  return (isnan(h) ? String(F("??")) : String(h,decimalPlaces)) + String(F("%"));
}

//Compute Heat Index
//Using both Rothfusz and Steadman's equations (http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml)
float Sensor::getHeatIndex(float temp, float hum) {
  if (isnan(temp))
    return NAN;
  float hi = 0.5 * (temp + 61.0 + ((temp - 68.0) * 1.2) + (hum * 0.094));

  if (hi > 79) {
    hi = -42.379 + 2.04901523 * temp + 10.14333127 * hum +
         -0.22475541 * temp * hum + -0.00683783 * pow(temp, 2) +
         -0.05481717 * pow(hum, 2) + 0.00122874 * pow(temp, 2) * hum +
         0.00085282 * temp * pow(hum, 2) + -0.00000199 * pow(temp, 2) * pow(hum, 2);

    if ((hum < 13) && (temp >= 80.0) && (temp <= 112.0))
      hi -= ((13.0 - hum) * 0.25) * sqrt((17.0 - abs(temp - 95.0)) * 0.05882);
    else if ((hum > 85.0) && (temp >= 80.0) && (temp <= 87.0))
      hi += ((hum - 85.0) * 0.1) * ((87.0 - temp) * 0.2);
  }

  return hi;
}

void Sensor::saveTempHist() {
  float t = pSensor->getTempF();
  if (isnan(t))
    return;
  Serial.print( F("Saving temp to history: "));
  Serial.println( t);
  for (uint8_t i = 0; i < TEMP_HIST_SIZE - 1; i++) { //move all values left
    _tempHistory[i] = _tempHistory[i+1];
    //Serial.println( "_tempHistory[" + String(i) + "] " + String(_tempHistory[i]) + "->" + String(tempF2C(int2Float(_tempHistory[i]))));
  }
  _tempHistory[TEMP_HIST_SIZE - 1] = Sensor::float2Int(t); //save the latest value
}

void drawSensorError( OLEDDisplay *display, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString((display->getWidth() / 2) + x, 21 + y, getStr(s_Temperature_sensor_error));
}

void drawSensor(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (isnan(pSensor->getTempF())) {
    drawSensorError( display, x ,y);
    return;
  }
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 6 + y, getStr(s_INDOOR));

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(8 + x, 36 + y, getStr(s_feel) + Sensor::strTemp(pSensor->getHeatIndex()));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(120 + x, 36 + y, getStr(s_hum));

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);

  display->drawString(8 + x, 15 + y, Sensor::strTemp(pSensor->getTempF(), DEC_PLACES));
  display->drawString(80 + x, 15 + y, Sensor::strHum(pSensor->getHum(), DEC_PLACES));

  display->setFont(Meteocons_Plain_21);
  display->drawString(-7 + x, 19 + y, F("'")); //show thermometer symbol
}
