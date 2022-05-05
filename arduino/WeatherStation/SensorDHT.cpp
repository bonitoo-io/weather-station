#include "SensorDHT.h"
#include "Tools.h"
#include "WeatherStation.h"

// Internal sensor settings
#define PIN_DHT D1      // Digital pin connected to the DHT11 sensor - GPIO5

float SensorDHT::driverGetTempF() {
  if (_readSensor())
    return Sensor::tempC2F((float)_celsius10 / 10.0);
  else
    return NAN;
}

float SensorDHT::driverGetHum( bool secondRead) {
  if (secondRead || _readSensor())
    return (float)_humidity10 / 10.0;
  else
    return NAN;
}

bool SensorDHT::_readSensor() {
  uint8_t data[5];

  pinMode(PIN_DHT, INPUT_PULLUP); // Go into high impedance state
  delay(1);

  pinMode(PIN_DHT, OUTPUT); // First set data line low for a period
  digitalWrite(PIN_DHT, LOW);
  pulseIn(PIN_DHT, HIGH, 20000);
  digitalWrite(PIN_DHT, HIGH);

  pinMode(PIN_DHT, INPUT);

  // Turn off interrupts temporarily because the next sections
  // are timing critical and we don't want any interruptions
  InterruptLock lock;

  if (!pulseIn( PIN_DHT, HIGH, 300)) { //timeout
    return false;
  }

  // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
  // microsecond low pulse followed by a variable length high pulse.  If the
  // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
  // then it's a 1. We measure the cycle count of the initial 50us low pulse
  // and use that to compare to the cycle count of the high pulse to determine
  // if the bit is a 0 (high state cycle count < low state cycle count), or a
  // 1 (high state cycle count > low state cycle count).
  for (uint8_t i = 0; i < 40; i++)
    data[i / 8] += data[i / 8] + (_expectPulse( LOW) > _expectPulse( HIGH));

  if (data[4] != uint8_t(data[0] + data[1] + data[2] + data[3])) { //check checksum
    Serial.println( F("DHT11 checksum error"));
    return false;
  }

  //convert to temperature
  _celsius10 = data[2];
  if (data[3] & 0x80) //handle sign
    _celsius10 = -1 - _celsius10;
  _celsius10 = _celsius10 * 10;
   _celsius10 += (data[3] & 0x0f);

  //convert to humidity
  _humidity10 = (data[0] * 10) + data[1];

  return true;
}

uint16_t SensorDHT::_expectPulse(int state) {
  uint16_t cycle = microsecondsToClockCycles(200);  //200 microseconds
  while (digitalRead(PIN_DHT) == state) {
    cycle--;
    if (cycle == 0)
      return 0; //timeout
  }
  return cycle;
}