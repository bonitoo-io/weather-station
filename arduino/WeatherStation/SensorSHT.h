#ifndef STHC3_SENSOR_H
#define STHC3_SENSOR_H

#include <Arduino.h>
#include <SHTSensor.h>
#include "Sensor.h"

class SensorSHT : public Sensor {
public:
  SensorSHT() : _sht(SHTSensor::AUTO_DETECT){};
protected:
  virtual bool driverSetup() override;
  virtual const __FlashStringHelper * driverName() override; //get sensor name
  virtual float driverGetTempF() override; //temperatures always in fahrenheit
  virtual float driverGetHum( bool secondRead) override;  //humidity always in percent
private:
  SHTSensor _sht; //sensor autodetection
};

#endif //STHC3_SENSOR_H
