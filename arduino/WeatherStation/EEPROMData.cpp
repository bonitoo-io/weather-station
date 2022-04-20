#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROMData.h"


#define EEPROM_DATA_MAGIC 212

EEPROMData::EEPROMData() {
  _data.magic = EEPROM_DATA_MAGIC;
  _data.tempOffset = 0;
  _data.humOffset = 0;
  _dirty = false;
}

void EEPROMData::begin() {
  EEPROM.begin(sizeof(_data));
}

void EEPROMData::setTempOffset(float tempOffset) { 
  Serial.printf_P(PSTR("EEPROMData: set temp %.1f\n"), tempOffset);
  if(_data.tempOffset != tempOffset) {
    _data.tempOffset = tempOffset;   
    _dirty = true;
  }
}

void EEPROMData::setHumOffset(float humOffset) { 
  Serial.printf_P(PSTR("EEPROMData: set hum %.1f\n"), humOffset);
  if(_data.humOffset != humOffset) {
    _data.humOffset = humOffset;   
    _dirty = true;
  }
}

uint8_t EEPROMData::write() {
  Serial.print(F("EEPROMData:write "));
  if(_dirty) {
    EEPROM.put(0, _data);
    EEPROM.commit();
    Serial.println(F(" ok"));
  } else {
    Serial.println(F(" not changed"));
  }
  return 0;
}

uint8_t EEPROMData::read() {
  Serial.print(F("EEPROMData:read "));
  uint8_t magic = EEPROM.read(0);
  if(magic == EEPROM_DATA_MAGIC) {
    EEPROM.get(0, _data);
    Serial.printf_P(PSTR(" read temp %.1f, hum %.1f\n"), _data.tempOffset, _data.humOffset);
    return 0;
  } 
  Serial.println(F(" not valid"));
  return -1;
}
