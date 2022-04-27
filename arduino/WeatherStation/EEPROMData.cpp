#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROMData.h"
#include "Sensor.h"

#define EEPROM_DATA_MAGIC 213

EEPROMData::EEPROMData() {
  _data.magic = EEPROM_DATA_MAGIC;
  _data.tempOffset = 0;
  _data.humOffset = 0;
}

void EEPROMData::begin() {
  EEPROM.begin(sizeof(_data));
}

float EEPROMData::getTempOffsetRaw() {
  return Sensor::int2Float(_data.tempOffset);
}

void EEPROMData::setTempOffsetRaw(float tempOffset) {
  _data.tempOffset = Sensor::float2Int(tempOffset);
}

float EEPROMData::getHumOffsetRaw() {
  return Sensor::int2Float( _data.humOffset);
}

void EEPROMData::setHumOffsetRaw(float humOffset) {
  _data.humOffset = Sensor::float2Int(humOffset);
}

void EEPROMData::_printData() {
  Serial.printf_P(PSTR(" temp %d (%.2f C), hum %d (%.2f %%)\n"), _data.tempOffset, Sensor::int2Float(_data.tempOffset) * 5.0 / 9.0, _data.humOffset, Sensor::int2Float(_data.humOffset));
}

uint8_t EEPROMData::write() {
  Serial.print(F("EEPROMData:write"));
  _printData();
  EEPROM.put(0, _data);
  EEPROM.commit();
  return 0;
}

uint8_t EEPROMData::read() {
  Serial.print(F("EEPROMData:read"));
  uint8_t magic = EEPROM.read(0);
  if(magic == EEPROM_DATA_MAGIC) {
    EEPROM.get(0, _data);
    _printData();
    return 0;
  }
  Serial.println(F(" not valid"));
  return -1;
}
