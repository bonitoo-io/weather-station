#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>

#include "tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

extern tConfig conf;

#define MAX_FORECASTS 3

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;

float getCurrentWeatherTemperature() {
  return currentWeather.temp;
}

void updateCurrentWeather( const bool metric, const String lang, const String location, const String APIKey) {
  currentWeatherClient.setMetric(metric);
  currentWeatherClient.setLanguage(lang);
  currentWeatherClient.updateCurrent(&currentWeather, APIKey, location);
}

void updateForecast( const bool metric, const String lang, const String location, const String APIKey) {
  forecastClient.setMetric(metric);
  forecastClient.setLanguage(lang);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecasts(forecasts, APIKey, location, MAX_FORECASTS);  
}


void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth() + x, 7 + y, utf8ascii(conf.location));
  display->drawString(display->getWidth() + x, 38 + y, "(" + String(currentWeather.tempMin, 0) + "-" + String(currentWeather.tempMax, 0) + (conf.useMetric ? "°C" : "°F") + ")");
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 38 + y, utf8ascii(currentWeather.description));
  display->drawString(38 + x, 17 + y, " wind");
  display->drawString(38 + x, 27 + y, String(currentWeather.windSpeed, 0) + (conf.useMetric ? "m/s" : "mph"));

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth() + x, 16 + y, String(currentWeather.temp, 0) + (conf.useMetric ? "°C" : "°F"));

  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 4 + y, currentWeather.iconMeteoCon);
}


void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 5, WDAY_NAMES[timeInfo->tm_wday]);
  display->drawString(x + 20, y + 39, String(forecasts[dayIndex].temp, 0) + (conf.useMetric ? "°C" : "°F"));
  
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 17, forecasts[dayIndex].iconMeteoCon);
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
}


void drawWindForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 5, WDAY_NAMES[timeInfo->tm_wday]);
  
  const int clockSize=10;
  int clockCenterX=21+x;
  int clockCenterY=28+y;

  // Draw marks for hours
  for (int i=0; i<8; i++) {
    float f = ((i * 45) + 270) * 0.0175;  //angle to radians
    display->drawLine(clockSize*cos(f)+clockCenterX, clockSize*sin(f)+clockCenterY, (clockSize-2+(i%2==0?0:1))*cos(f)+clockCenterX, (clockSize-2+(i%2==0?0:1))*sin(f)+clockCenterY);
  }
  float w = ((forecasts[dayIndex].windDeg)+270)*0.0175;
  display->drawLine(clockSize*cos(w)+clockCenterX, clockSize*sin(w)+clockCenterY, cos(w)+clockCenterX, sin(w)+clockCenterY);
  
  display->setFont(ArialMT_Plain_10);
  //display->drawString(x + 20, y + 29, String(forecasts[dayIndex].windDeg, 0) + "°");
  display->drawString(x + 20, y + 39, String(forecasts[dayIndex].windSpeed, 0) + (conf.useMetric ? "m/s" : "mph"));
}

void drawWindForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawWindForecastDetails(display, x, y, 0);
  drawWindForecastDetails(display, x + 44, y, 1);
  drawWindForecastDetails(display, x + 88, y, 2);
}

void showFont(OLEDDisplay *display) {
  while (true)
  for (char i=0; i<225; i++) {
    display->clear();
    display->setFont(Meteocons_Plain_21);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 0, String((char)(i+32)));
    display->setFont(ArialMT_Plain_10);
    display->drawString(0, 32, String((int)(i+32)) + " " + String((char)(i+32)));
    display->display();
    delay( 1000);
  }
}
