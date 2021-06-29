#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H


void setupDHT();
float getDHTTemp(bool metric);
float getDHTHum();
void saveDHTTempHist(bool metric);
void refreshDHTCachedValues(bool metric);
float getDHTCachedTemp();
float getDHTCachedHum();

#endif //DHT_SENSOR_H