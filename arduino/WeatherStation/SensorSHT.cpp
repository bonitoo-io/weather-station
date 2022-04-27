#include "SensorSHT.h"

bool SensorSHT::driverSetup() {
  Wire.begin();

  if (!_sht.init()) {
    Serial.println( F("Sensor SHTxx init failed"));
    return false;
  }
  _sht.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH); // only supported by SHT3x
  return true;
}

const __FlashStringHelper * SensorSHT::driverName() {
  switch ( _sht.mSensorType) {
    case SHTSensor::SHT3X:
    case SHTSensor::SHT3X_ALT:
      return F("SHT3x");
    case SHTSensor::SHTC1:
      return F("SHTCx");
    case SHTSensor::SHT4X:
      return F("SHT4x");
    default:
      return F("SHTxx");    
  }
}

float SensorSHT::driverGetTempF() {
  if (_sht.readSample())
    return Sensor::tempC2F(_sht.getTemperature());
  else
    return NAN;
}

float SensorSHT::driverGetHum( bool secondRead) {
  if (_sht.readSample())
    return _sht.getHumidity();
  else
    return NAN;
}
