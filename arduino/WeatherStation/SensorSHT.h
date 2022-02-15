#ifndef STHC3_SENSOR_H
#define STHC3_SENSOR_H

#include <Arduino.h>
#include <SHTSensor.h>
#include "Sensor.h"

class SensorSHT : public Sensor {
public:  
  ~SensorSHT();
  static bool driverDetect();
protected:
  virtual bool driverSetup();
  virtual const __FlashStringHelper * driverName() { return F("SHTC3");} ; //get sensor name
  virtual float driverGetTemp(); //temperatures always in fahrenheit
  virtual float driverGetHum( bool secondRead);  //humidity always in percent
private:
  SHTSensor* _pSht = nullptr;
};

#endif //STHC3_SENSOR_H
