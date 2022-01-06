#include <OLEDDisplayUi.h>
#include "DHTSensor.h"
#include "Tools.h"
#include "WeatherStationFonts.h"

int getCovid19Dispersal() {
  //Inputs based on https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7229913/
  float t = getDHTTemp( true);
  float h = getDHTHum();

  if ((t <= 3) || (t >= 20))
    t = 0; //low probability
  else
    if ((t > 8) && (t < 15))
      t = 2; //high probability
    else
      t = 1; //medium probability

  if ((h <= 40) || (h >= 88))
    h = 0; //low probability
  else
    if ((h > 54) && (h < 76))
      h = 2; //high probability
    else
      h = 1; //medium probability
  //return (int)((t+h) / 2);
  return (int)(100 + (t * 10) + h);
}


void drawCovid19(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x + 40, y + 4,  "Covid-19 spread risk");
  display->drawString(x + 40, y + 13,  String(getCovid19Dispersal()));

  display->setFont(MoonPhases_Regular_36);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 5 + y, "A");
}
