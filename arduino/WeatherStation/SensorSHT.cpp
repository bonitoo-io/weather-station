#include "SensorSHT.h"
#define TEMP_SENSOR SHTSensor::SHTC3

bool SensorSHT::detect() {
  Wire.begin();
  Wire.beginTransmission(0x70);
  return Wire.endTransmission() == 0;  
}

bool SensorSHT::_setup() {
  _sht = new SHTSensor(TEMP_SENSOR);
  if (!_sht)
    return false;
  if (!_sht->init())
    return false;
  _sht->setAccuracy(SHTSensor::SHT_ACCURACY_HIGH); // only supported by SHT3x    
  return true;
}

float SensorSHT::_getTemp() {
  if (_sht && _sht->readSample())
    return Sensor::tempC2F(_sht->getTemperature());
  else
    return NAN;
}

float SensorSHT::_getHum() {
  if (_sht && _sht->readSample())
    return _sht->getHumidity();
  else
    return NAN;
}

SensorSHT::~SensorSHT() { 
  if (_sht) 
    delete _sht;
  _sht = nullptr;  
}
