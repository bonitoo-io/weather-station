#ifndef SENSOR_H
#define SENSOR_H

#define NO_VALUE_INT -32768
#define TEMP_HIST_SIZE 90

template <typename T>
class Median3Filter  {
public:
  void init( T sample = 0) {
    _samples[0]= _samples[1]= _samples[2]=sample;
    Serial.println( "Filter init " + String(sample));
  }

  T medianFilter(T sample) {
    _samples[0] = _samples[1];
    _samples[1] = _samples[2];
    _samples[2] = sample;
    return getValue();
  }

  T getValue() {
    T median;
    if (_samples[2] < _samples[1]) {
      if (_samples[2] < _samples[0]) {
        if (_samples[1] < _samples[0])
          median = _samples[1];
        else
          median = _samples[0];
      }
      else
        median = _samples[2];
    } else {
      if (_samples[2] < _samples[0])
        median = _samples[2];
      else {
        if (_samples[1] < _samples[0])
          median = _samples[0];
        else
          median = _samples[1];
      }
    }
    return median;
  }
private:
  T _samples[3];
};

class Sensor {
public:
  static bool setupSensor();
  bool setup();
  float getTempF( bool forceCached = false);
  float getHum( bool forceCached = false);
  float inline getTempForceC( bool forceCached = false) {return tempF2C(getTempF( forceCached));};
  float getHeatIndex( bool forceCached = false) { return getHeatIndex( getTempF(forceCached), getHum(forceCached));} ;
  void saveTempHist();
  int16_t getHist( uint8_t pos);
  inline void setHist( uint8_t pos, int16_t data) { if (pos < TEMP_HIST_SIZE) _tempHistory[pos]=data;};
  inline int16_t getRawHist(uint8_t pos) { return _tempHistory[pos];}
  inline const __FlashStringHelper * getSensorName() { return driverName();}

//Basic temperature and humidity handling functions (static - can be used without class instance)
  static int16_t temp2Int( float temp, bool metric);
  static float int2Temp( int16_t temp, bool metric) {return metric ? tempF2C(int2Float(temp)) : int2Float(temp);};
  static int16_t float2Int( float f) { return isnan(f) ? NO_VALUE_INT : round( f * 10.0);};
  static float int2Float( int16_t i) { return i == NO_VALUE_INT ? NAN : ((float)i) / 10.0;};
  static float tempF2C(float f) { return isnan(f) ? NAN : (f - 32.0) * 5.0 / 9.0; };
  static float tempC2F(float c) { return isnan(c) ? NAN : (c * 9.0 / 5.0) + 32.0; };
  static float getHeatIndex(float temp, float hum);
  static String strTempUnit();
  static        String strTempValue( float t, uint8_t decimalPlaces = 0);
  static inline String strTempValueInt( int16_t t) { return strTempValue(int2Temp( t, false));};
  static inline String strTemp( float t, uint8_t decimalPlaces = 0) { return strTempValue(t, decimalPlaces) + strTempUnit();};
  static inline String strTempInt( int16_t t, bool metric) { return strTemp(int2Temp( t, metric));};
  static inline String strTempInt( int16_t t) { return strTemp(int2Temp( t, false));};
  static String strHum( float h, uint8_t decimalPlaces = 0);
  static inline String strHumInt( int16_t h) { return strHum( int2Float( h));};
  void resetTempFilter( float t = NAN);
  void resetHumFilter( float h = NAN);
protected:
//virtual functions for temperature/humidity sensors
  virtual bool driverSetup() = 0;  //setup the sensor, allocate sensor class
  virtual const __FlashStringHelper * driverName() = 0; //get sensor name
  virtual float driverGetTempF() = 0; //temperatures always in fahrenheit
  virtual float driverGetHum( bool secondRead) = 0;  //humidity always in percent
  virtual inline uint16_t driverGetMaxRefreshRateMs() { return 500;};  //default refresh is 0.5s (may be overwritten)
private:
  void internalLoadHum( bool secondRead);
  Median3Filter<float> _tempFilt;
  Median3Filter<int16_t> _humFilt;
  int16_t _tempHistory[TEMP_HIST_SIZE];
  unsigned long _timeNextUpdate;
};


extern Sensor* pSensor;

class OLEDDisplay;
class OLEDDisplayUiState;
void drawSensor(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawSensorError( OLEDDisplay *display, int16_t x, int16_t y);
#endif //SENSOR_H
