#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <SunMoonCalc.h>

#include "tools.h"
#include "WeatherStationFonts.h"

extern OpenWeatherMapCurrentData currentWeather;

SunMoonCalc::Result sunMoonData;
char moonAgeImage;

void updateAstronomy(bool firstStart, const float lat, const float lon) {
  // 'now' has to be UTC, lat/lng in degrees not raadians
  SunMoonCalc smCalc = SunMoonCalc(time(nullptr), lat, lon);
  sunMoonData = smCalc.calculateSunAndMoonData();
  moonAgeImage = (char) (65 + ((uint8_t) ((uint8_t)(26 * sunMoonData.moon.age / 30) % 26)));
}


void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x + 38, y + 10, "Sun:" + strTime(currentWeather.sunrise, true) + "-" + strTime(currentWeather.sunset, true));
  display->drawString(x + 38, y + 23, "Moon:" + strTime(sunMoonData.moon.rise, true) + "-" + strTime(sunMoonData.moon.set, true));
  display->drawString(x + 38, y + 35, String( MOON_PHASES[sunMoonData.moon.phase.index]));

  display->setFont(MoonPhases_Regular_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 7 + y, String(moonAgeImage));
}
