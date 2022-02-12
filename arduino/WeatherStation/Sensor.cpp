#include <OLEDDisplayUi.h>
#include "WeatherStationFonts.h"
#include "tools.h"
#include "WeatherStation.h"
#include "Sensor.h"
#include "SensorDHT.h"

Sensor* pSensor = nullptr;

bool setupSensor() {
  //prepare lastupdate -> test i2c adress -> initialize Sensor class
  pSensor = new SensorDHT();
  if (pSensor)
    pSensor->setup(); //initialize sensor 
  return true;
}

bool Sensor::setup() {
  _setup(); //initialize sensor
  Serial.println( String(F("Temp raw = ")) + String(_getTemp()));
  float h = _getHum();
  Serial.println( String(F("Humidity calib = ")) + String(h));
  //fix humidity offset overflow
  if ((station.getAdvancedSettings()->humOffset != 0) && (station.getAdvancedSettings()->humOffset + h > 100)) { 
    station.getAdvancedSettings()->humOffset = 0;
    Serial.println( F("DHT humidity overflow, zeroing offset"));
  }
  //autocalibrate sensors with faulty humidity
  if (( h < 36) && (station.getAdvancedSettings()->humOffset == 0)) { 
    station.getAdvancedSettings()->humOffset = 55 - h;
    Serial.print( F("DHT humidity autocalibration offset: "));
    Serial.println( station.getAdvancedSettings()->humOffset);
  }
  _tempFilt.init( _getTemp() + (station.getRegionalSettings()->useMetricUnits ? station.getAdvancedSettings()->tempOffset * 9.0 / 5.0 : station.getAdvancedSettings()->tempOffset)); //prepare filter data
  _humFilt.init( h + station.getAdvancedSettings()->humOffset); //prepare filter data
  //clean data
  for (uint8_t i = 0; i < TEMP_HIST_SIZE; i++)
    _tempHistory[i] = NO_VALUE_INT;
  return true;  
}

float Sensor::getTemp( bool forceCached) {
  if (forceCached || (_timeNextUpdate >= millis()))
    return _tempFilt.getValue();
  
  float t = _getTemp(); //read temperature from the sensor
  //Serial.println( "Temperature = " + String(t) + "->" + String(tempF2C(t)));
  if (isnan(t)) {
    Serial.println( F("Received NAN temperature!"));
    return _tempFilt.getValue();  //restore old value
  }
  _timeNextUpdate = millis() + _getMaxRefreshRateMs();    //next time to read metrics
  _loadHum(); //also process humidity
  
  if (station.getAdvancedSettings()->tempOffset != 0) //Add offset
    t += station.getRegionalSettings()->useMetricUnits ? station.getAdvancedSettings()->tempOffset * 9.0 / 5.0 : station.getAdvancedSettings()->tempOffset; 

  return _tempFilt.medianFilter(t);
}

void Sensor::_loadHum() {
  float h = _getHum(); //read humidity from the sensor
  //Serial.println( "Humidity = " + String(h));
  if (isnan(h)) {
    Serial.println( F("Received NAN humidity!"));
    return;
  }
  h += station.getAdvancedSettings()->humOffset; //Add offset
  if (h > 100) {
    h = 100;
  } else if (h < 0) {
    h = 0;
  }
  _humFilt.medianFilter(float2Int(h)); 
}

float Sensor::getHum( bool forceCached) {
  if (!(forceCached || (_timeNextUpdate >= millis())))
    _loadHum();
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

String Sensor::strTempValue( float t) {
  if (station.getRegionalSettings()->useMetricUnits)  //convert to C, if needed
    t = tempF2C(t);
  return (isnan(t) ? String(F("??")) : String(t,0));
}

String Sensor::strHum( float h) {
  return (isnan(h) ? String(F("??")) : String(h,0)) + String(F("%"));  
}

//Compute Heat Index
//Using both Rothfusz and Steadman's equations (http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml)
float Sensor::getHeatIndex(float temp, float hum) {
  if (isnan(temp))
    return NAN;
  float hi;
  if (hi > 79) {    
    if ((hum < 13) && (temp >= 80.0) && (temp <= 112.0))
      hi -= ((13.0 - hum) * 0.25) * sqrt((17.0 - abs(temp - 95.0)) * 0.05882);
    else if ((hum > 85.0) && (temp >= 80.0) && (temp <= 87.0))
      hi += ((hum - 85.0) * 0.1) * ((87.0 - temp) * 0.2);
    else
      hi = -42.379 + 2.04901523 * temp + 10.14333127 * hum +
         -0.22475541 * temp * hum + -0.00683783 * pow(temp, 2) +
         -0.05481717 * pow(hum, 2) + 0.00122874 * pow(temp, 2) * hum +
         0.00085282 * temp * pow(hum, 2) + -0.00000199 * pow(temp, 2) * pow(hum, 2);
  } else
    hi = 0.5 * (temp + 61.0 + ((temp - 68.0) * 1.2) + (hum * 0.094));

  return hi;
}

void Sensor::saveTempHist() {
  float t = pSensor->getTemp();
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
  if (isnan(pSensor->getTemp())) {
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
  
  display->drawString(8 + x, 15 + y, Sensor::strTemp(pSensor->getTemp()));
  display->drawString(80 + x, 15 + y, Sensor::strHum(pSensor->getHum()));

  display->setFont(Meteocons_Plain_21);
  display->drawString(-7 + x, 19 + y, F("'")); //show thermomether symbol
}
