#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <arduino.h>
#include <SDHT.h>
#include "Sensor.h"

class SensorDHT : public Sensor {
public:  
protected:
  virtual bool _setup() { return true;};
  virtual float _getTemp(); //temperatures always in fahrenheit
  virtual float _getHum();  //humidity always in percent
  virtual inline uint16_t _getMaxRefreshRateMs() { return 2000;};  //DHT11 max 1Hz, set 2s
private:
  SDHT _dht;
};

#endif //DHT_SENSOR_H
