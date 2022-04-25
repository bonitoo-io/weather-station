#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <arduino.h>
#include <SDHT.h>
#include "Sensor.h"

class SensorDHT : public Sensor {
protected:
  virtual bool driverSetup() override { return true;}; //no reliable detection available - sensor with lowest priority
  virtual const __FlashStringHelper * driverName() override { return F("DHT11");} ; //get sensor name
  virtual float driverGetTempF() override ; //temperatures always in fahrenheit
  virtual float driverGetHum( bool secondRead) override;  //humidity always in percent
  virtual inline uint16_t driverGetMaxRefreshRateMs() override { return 2000;};  //DHT11 max 1Hz, set 2s
private:
  SDHT _dht;
};

#endif //DHT_SENSOR_H
