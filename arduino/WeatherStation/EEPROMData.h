#ifndef EEPROM_DATA_H
#define EEPROM_DATA_H


class EEPROMData {
  private:
    struct  {
      uint8_t magic;
      float tempOffset;
      float humOffset;
    } _data;
    bool _dirty;
  public:
    EEPROMData();

    void begin();
    
    float getTempOffset() { return  _data.tempOffset; }
    void setTempOffset(float tempOffset);
    float getHumOffset() { return  _data.humOffset; }
    void setHumOffset(float humOffset);

    uint8_t write();
    uint8_t read();
};

#endif //EEPROM_DATA_H