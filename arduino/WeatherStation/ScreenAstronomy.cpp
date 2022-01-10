#include <OLEDDisplayUi.h>
#include <SunMoonCalc.h>

#include "Tools.h"
#include "WeatherStationFonts.h"

extern tCurrentWeather currentWeather;

time_t moonRise;
time_t moonSet;
uint8_t moonPhase;
char moonAgeImage;

bool updateAstronomy(bool firstStart, const float lat, const float lon) {
  // 'now' has to be UTC, lat/lng in degrees not raadians
  SunMoonCalc smCalc = SunMoonCalc(time(nullptr), lat, lon);
  SunMoonCalc::Result sunMoonData = smCalc.calculateSunAndMoonData();
  
  moonRise = sunMoonData.moon.rise;
  moonSet = sunMoonData.moon.set;
  moonPhase = sunMoonData.moon.phase.index;
  moonAgeImage = (char) (65 + 26 * (((uint8_t)(15 + sunMoonData.moon.age) % 30) / 30.0));
  if ( currentWeather.temp == 0xffff) {
    currentWeather.sunrise = sunMoonData.sun.rise;
    currentWeather.sunset = sunMoonData.sun.set;
  }
  return true;
}


void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x + 40, y + 4,  getStr(s_Sun));
  display->drawString(x + 43, y + 13, strTime(currentWeather.sunrise, true) + strTimeSuffix(currentWeather.sunrise) + String(F("-")) + strTime(currentWeather.sunset, true) +  strTimeSuffix(currentWeather.sunset));
  display->drawString(x + 40, y + 21, getStr(s_Moon));
  display->drawString(x + 43, y + 30, strTime(moonRise, true) + strTimeSuffix(moonRise) + "-" + strTime(moonSet, true) + strTimeSuffix(moonSet));
  display->drawString(x + 0, y + 36, getMoonPhaseName( moonPhase));

  display->setFont(MoonPhases_Regular_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 3 + y, String(moonAgeImage));
}
