#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <SunMoonCalc.h>

#include "tools.h"
#include "WeatherStationFonts.h"

extern OpenWeatherMapCurrentData currentWeather;

time_t moonRise;
time_t moonSet;
uint8_t moonPhase;
char moonAgeImage;

void updateAstronomy(bool firstStart, const float lat, const float lon) {
  // 'now' has to be UTC, lat/lng in degrees not raadians
  SunMoonCalc smCalc = SunMoonCalc(time(nullptr), lat, lon);
  SunMoonCalc::Result sunMoonData = smCalc.calculateSunAndMoonData();
  
  moonRise = sunMoonData.moon.rise;
  moonSet = sunMoonData.moon.set;
  moonPhase = sunMoonData.moon.phase.index;
  moonAgeImage = (char) (65 + 26 * (((uint8_t)(15 + sunMoonData.moon.age) % 30) / 30.0));
}


void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x + 38, y + 10, "Sun:" + strTime(currentWeather.sunrise, true) + "-" + strTime(currentWeather.sunset, true));
  display->drawString(x + 38, y + 23, "Moon:" + strTime(moonRise, true) + "-" + strTime(moonSet, true));
  display->drawString(x + 38, y + 35, String( MOON_PHASES[moonPhase]));

  display->setFont(MoonPhases_Regular_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 7 + y, String(moonAgeImage));
}
