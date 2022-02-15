#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <arduino.h>
#include <SDHT.h>
#include "Sensor.h"

class SensorDHT : public Sensor {
public:  
protected:
  virtual bool driverSetup() { return true;};
  virtual const __FlashStringHelper * driverName() { return F("DHT11");} ; //get sensor name
  virtual float driverGetTemp(); //temperatures always in fahrenheit
  virtual float driverGetHum( bool secondRead);  //humidity always in percent
  virtual inline uint16_t driverGetMaxRefreshRateMs() { return 2000;};  //DHT11 max 1Hz, set 2s
private:
  SDHT _dht;
};

#endif //DHT_SENSOR_H
