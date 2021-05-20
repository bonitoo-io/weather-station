#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>

#include "tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

extern bool g_bMetric;
extern String g_location;

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
  display->drawString(display->getWidth() + x, 10 + y, utf8ascii(g_location));
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, utf8ascii(currentWeather.description));
 

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth() + x, 18 + y, String(currentWeather.temp, 0) + (g_bMetric ? "°C" : "°F"));

  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 4 + y, currentWeather.iconMeteoCon);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 5, WDAY_NAMES[timeInfo->tm_wday]);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 17, forecasts[dayIndex].iconMeteoCon);

  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 39, String(forecasts[dayIndex].temp, 0) + (g_bMetric ? "°C" : "°F"));
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
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
