#include <OLEDDisplayUi.h>
#include <OpenWeatherMapCurrent.h>
#include <Astronomy.h>
#include <SunMoonCalc.h>

#include "tools.h"
#include "WeatherStationFonts.h"

extern OpenWeatherMapCurrentData currentWeather;

Astronomy::MoonData moonData;
char moonAgeImage;


void updateAstronomy(OLEDDisplay *display, bool firstStart) {
  // 'now' has to be UTC, lat/lng in degrees not raadians
  //SunMoonCalc smCalc = SunMoonCalc(time(nullptr), 52.520008, 13.404954);
  //const SunMoonCalc::Result result = smCalc.calculateSunAndMoonData();

  Astronomy astronomy;
  moonData = astronomy.calculateMoonData(time(nullptr));
  const float lunarMonth = 29.53;
  uint8_t moonAge = moonData.phase <= 4 ? lunarMonth * moonData.illumination / 2 : lunarMonth - moonData.illumination * lunarMonth / 2;
  moonAgeImage = (char) (65 + ((uint8_t) ((26 * moonAge / 30) % 26)));
}


void drawAstronomy(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x + 40, y + 10, "Sun: " + strTime(currentWeather.sunrise, true) + " - " + strTime(currentWeather.sunset, true));
  display->drawString(x + 40, y + 25, "Moon: ");
  display->drawString(x + 40, y + 35, String( MOON_PHASES[moonData.phase]));

  display->setFont(MoonPhases_Regular_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 7 + y, String(moonAgeImage));
}
