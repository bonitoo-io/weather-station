#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>

#include "Tools.h"
#include "WeatherStation.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "RegionalSettings.h"

tCurrentWeather currentWeather;
#define MAX_FORECASTS 3
tForecast forecasts[MAX_FORECASTS];


int getCurrentWeatherTemperature() {
  return currentWeather.temp;
}


void updateCurrentWeather(RegionalSettings *pRegionalSettings, const String& APIKey) {
  OpenWeatherMapCurrent currentWeatherClient;
  OpenWeatherMapCurrentData _currentWeather;
  currentWeatherClient.setMetric(pRegionalSettings->useMetricUnits);
  currentWeatherClient.setLanguage(pRegionalSettings->language);
  _currentWeather.temp = NAN;
  currentWeatherClient.updateCurrent(&_currentWeather, APIKey, pRegionalSettings->location);
  if (isnan(_currentWeather.temp)) {
    currentWeather.temp =  0xffff;
    return;  
  }
  
  currentWeather.temp =  round( _currentWeather.temp);
  currentWeather.tempMin = round( _currentWeather.tempMin);
  currentWeather.tempMax = round( _currentWeather.tempMax);
  currentWeather.description = _currentWeather.description;
  currentWeather.windSpeed = round( _currentWeather.windSpeed);
  currentWeather.iconMeteoCon = _currentWeather.iconMeteoCon;
  currentWeather.sunrise = _currentWeather.sunrise;
  currentWeather.sunset = _currentWeather.sunset;
}


void updateForecast( RegionalSettings *pRegionalSettings, const String& APIKey) {
  OpenWeatherMapForecast forecastClient;
  OpenWeatherMapForecastData _forecasts[MAX_FORECASTS];
  _forecasts[0].temp = NAN;
  forecastClient.setMetric(pRegionalSettings->useMetricUnits);
  forecastClient.setLanguage(pRegionalSettings->language);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecasts(_forecasts, APIKey, pRegionalSettings->location, MAX_FORECASTS);

  if (isnan(_forecasts[0].temp)) {
    forecasts[0].temp = 0xffff;
    return;
  }

  for (unsigned int i = 0; i < MAX_FORECASTS; i++) {
    forecasts[i].observationTime = _forecasts[i].observationTime;
    forecasts[i].temp = round( _forecasts[i].temp);
    forecasts[i].iconMeteoCon = _forecasts[i].iconMeteoCon;
    forecasts[i].windDeg = round( _forecasts[i].windDeg);
    forecasts[i].windSpeed = round( _forecasts[i].windSpeed);
  }
}

void forecastError( OLEDDisplay *display, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString((display->getWidth() / 2) + x, 21 + y, getStr( s_Forecast_error));
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (currentWeather.temp == 0xffff) {
    forecastError( display, x ,y);
    return;
  }
  
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth() + x, 7 + y, utf8ascii(station.getRegionalSettings()->location));
  display->drawString(display->getWidth() + x, 38 + y, String(F("(")) + String(currentWeather.tempMin) + String(F("-")) + strTemp(currentWeather.tempMax) + String(F(")")));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 38 + y, utf8ascii(currentWeather.description));
  display->drawString(40 + x, 17 + y, getStr( s_wind));
  display->drawString(38 + x, 27 + y, strWind(currentWeather.windSpeed));

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(display->getWidth() + x, 16 + y, strTemp(currentWeather.temp));

  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 4 + y, String(currentWeather.iconMeteoCon));
}


void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(x + 20, y + 5, getDayName(timeInfo->tm_wday));
  display->drawString(x + 20, y + 39, strTemp(forecasts[dayIndex].temp));
  
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 17, String(forecasts[dayIndex].iconMeteoCon));
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (forecasts[0].temp == 0xffff) {
    forecastError( display, x ,y);
    return;
  }
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
}

void arrow(OLEDDisplay *display, int x1, int y1, int x2, int y2, int alength, int awidth) {
  float distance;
  int dx, dy, x2o,y2o,x3,y3,x4,y4,k;
  distance = sqrt(pow((x1 - x2),2) + pow((y1 - y2), 2));
  dx = x2 + (x1 - x2) * alength / distance;
  dy = y2 + (y1 - y2) * alength / distance;
  k = awidth / alength;
  x2o = x2 - dx;
  y2o = dy - y2;
  x3 = y2o * k + dx;
  y3 = x2o * k + dy;
  //
  x4 = dx - y2o * k;
  y4 = dy - x2o * k;
  display->drawLine(x1, y1, x2, y2);
  display->drawLine(x1, y1, dx, dy);
  display->drawLine(x3, y3, x4, y4);
  display->drawLine(x3, y3, x2, y2);
  display->drawLine(x2, y2, x4, y4);
} 

void drawWindForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 5, getDayName(timeInfo->tm_wday));
  
  const int clockSize=10;
  int clockCenterX=21+x;
  int clockCenterY=28+y;
  float f;
  
  // Draw marks for hours
  for (unsigned int i=0; i<8; i++) {
    f = ((i * 45) + 270) * 0.0175;  //angle to radians
    if ( abs((int)((i * 45) + 270) - (int)(forecasts[dayIndex].windDeg+90)) > 40)
      display->drawLine(clockSize*cos(f)+clockCenterX, clockSize*sin(f)+clockCenterY, (clockSize-2+(i%2==0?0:1))*cos(f)+clockCenterX, (clockSize-1+(i%2==0?0:1))*sin(f)+clockCenterY);
  }
  f = (forecasts[dayIndex].windDeg+90)*0.0175;
  arrow( display, cos(f)+clockCenterX, sin(f)+clockCenterY, clockSize*cos(f)+clockCenterX, clockSize*sin(f)+clockCenterY, 3, 5);

  display->setFont(ArialMT_Plain_10);
  //display->drawString(x + 20, y + 29, String(forecasts[dayIndex].windDeg, 0) + "°");
  display->drawString(x + 20, y + 39, strWind(forecasts[dayIndex].windSpeed));
}

void drawWindForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (forecasts[0].temp == 0xffff) {
    forecastError( display, x ,y);
    return;
  }
  drawWindForecastDetails(display, x, y, 0);
  drawWindForecastDetails(display, x + 44, y, 1);
  drawWindForecastDetails(display, x + 88, y, 2);
}
