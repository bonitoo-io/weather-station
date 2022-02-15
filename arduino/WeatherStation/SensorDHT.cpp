#include "SensorDHT.h"
#include "Tools.h"
#include "WeatherStation.h"

// Internal sensor settings
#define DHT_TYPE DHT11  // Sensor DHT 11
#define PIN_DHT D1      // Digital pin connected to the DHT 11 sensor - GPIO5

float SensorDHT::driverGetTemp() {
  if (_dht.read(DHT_TYPE, PIN_DHT))
    return Sensor::tempC2F((float)_dht.celsius / 10.0);
  else
    return NAN;
}

float SensorDHT::driverGetHum( bool secondRead) {
  if (secondRead || _dht.read(DHT_TYPE, PIN_DHT))
    return (float)_dht.humidity / 10.0;
  else
    return NAN;
}
