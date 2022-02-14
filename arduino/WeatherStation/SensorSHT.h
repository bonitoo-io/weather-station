#ifndef STHC3_SENSOR_H
#define STHC3_SENSOR_H

#include <Arduino.h>
#include <SHTSensor.h>
#include "Sensor.h"

class SensorSHT : public Sensor {
public:  
  ~SensorSHT();
  static bool detect();
protected:
  virtual bool _setup();
  virtual float _getTemp(); //temperatures always in fahrenheit
  virtual float _getHum();  //humidity always in percent
private:
  SHTSensor* _sht = nullptr;
};

#endif //STHC3_SENSOR_H
