#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H


void setupDHT();
float getDHTTemp(bool metric);
float getDHTHum();
void saveDHTTempHist(bool metric);
void refreshDHTCachedValues(bool metric);
float getDHTCachedTemp();
float getDHTCachedHum();
class OLEDDisplay;
void sensorError( OLEDDisplay *display, int16_t x, int16_t y);
extern int16_t tempHistory[90];

#endif //DHT_SENSOR_H
