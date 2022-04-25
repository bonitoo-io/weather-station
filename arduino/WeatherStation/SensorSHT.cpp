#include "SensorSHT.h"
#define TEMP_SENSOR SHTSensor::SHTC3

bool SensorSHT::driverDetect() {
  Wire.begin();
  Wire.beginTransmission(0x70);
  return Wire.endTransmission() == 0;
}

bool SensorSHT::driverSetup() {
  _pSht = new SHTSensor(TEMP_SENSOR);
  if (!_pSht)
    return false;
  if (!_pSht->init())
    return false;
  _pSht->setAccuracy(SHTSensor::SHT_ACCURACY_HIGH); // only supported by SHT3x
  return true;
}

float SensorSHT::driverGetTempF() {
  if (_pSht && _pSht->readSample())
    return Sensor::tempC2F(_pSht->getTemperature());
  else
    return NAN;
}

float SensorSHT::driverGetHum( bool secondRead) {
  if (_pSht && _pSht->readSample())
    return _pSht->getHumidity();
  else
    return NAN;
}

SensorSHT::~SensorSHT() {
  if (_pSht)
    delete _pSht;
  _pSht = nullptr;
}
