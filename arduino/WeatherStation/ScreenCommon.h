#ifndef WS_SCREEN_COMMON_H
#define WS_SCREEN_COMMON_H

#include <OLEDDisplayUi.h>
#include "WiFi.h"
#include "InfluxDBHelper.h"
#include "AdvancedSettings.h"

class ScreenConstants { 
 public:
  static const char About; 
  static const char DateTimeAnalog;
  static const char DateTimeDigital;
  static const char SensorValues;
  static const char Covid19;
  static const char CurrentWeather;
  static const char WeatherForecast;
  static const char WindForecast;
  static const char Astronomy;
  static const char TemperatureChart;
  static const char Config;

  static String getDefaultList();
};


int16_t getCurrentWeatherTemperature();

void initOLEDUI(OLEDDisplayUi *ui, AdvancedSettings *pAdvancedSettings);
void configureUI(OLEDDisplayUi *ui, AdvancedSettings *pAdvancedSettings);
void drawUpdateProgress(OLEDDisplay *display, int percentage, const String& label);
void startWifiProgress(OLEDDisplay *display, const char* version, const char *ssid);
void drawWifiProgress(OLEDDisplay *display, const char* version, const char *ssid);
void drawAPInfo(OLEDDisplay *display, APInfo *info);
void drawFWUpdateInfo(OLEDDisplay *display, const String &fistLine, const String &secondLine);
void drawFWUpdateProgress(OLEDDisplay *display, const char* version, int percent);
void showConfiguration(OLEDDisplay *display, int secToReset, const char* version, long lastUpdate, const char *deviceID, InfluxDBHelper *influxDBHelper);

void drawAbout(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawWindForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawTemperatureChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCovid19(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
bool isInfluxDBError();


#endif //WS_SCREEN_COMMON_H
