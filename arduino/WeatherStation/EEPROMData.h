#ifndef EEPROM_DATA_H
#define EEPROM_DATA_H


class EEPROMData {
  private:
    struct  {
      uint8_t magic;
      int16_t tempOffset; // Temperature compenstation coefficient in fahrenheit (int * 10)
      int16_t humOffset; // Humidity compenstation coefficient (int * 10)
    } _data;
    void _printData();
  public:
    EEPROMData();

    void begin();
    
    float getTempOffsetRaw(); //Get temperature compenstation coefficient in fahrenheit
    void setTempOffsetRaw(float tempOffset);  //Set temperature compenstation coefficient in fahrenheit
    float getHumOffsetRaw(); //Get humidity compenstation coefficient
    void setHumOffsetRaw(float humOffset); //Set humidity compenstation coefficient

    uint8_t write();
    uint8_t read();
};

#endif //EEPROM_DATA_H
