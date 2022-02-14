#include "SensorDHT.h"
#include "Tools.h"
#include "WeatherStation.h"

// Internal sensor settings
#define DHT_TYPE DHT11  // Sensor DHT 11
#define PIN_DHT D1      // Digital pin connected to the DHT 11 sensor - GPIO5

bool SensorDHT::_setup() {
  _dht = new DHT(PIN_DHT, DHT_TYPE);
  if (!_dht)
    return false;
  _dht->begin();
  if ( isnan(_dht->readTemperature()))
    return false;
  return true;  
}

float SensorDHT::_getTemp() {
  if (_dht)
    return _dht->readTemperature(true);  //always return fahrenheit
  else
    return NAN;
}

float SensorDHT::_getHum() {
  if (_dht)
    return _dht->readHumidity();
  else
    return NAN;
}

SensorDHT::~SensorDHT() { 
  if (_dht) 
    delete _dht;
  _dht = nullptr;  
}
