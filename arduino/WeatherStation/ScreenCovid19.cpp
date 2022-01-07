#include <OLEDDisplayUi.h>
#include "DHTSensor.h"
#include "Tools.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

uint8_t getCovid19Dispersal() {
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
  return (uint8_t)((t+h) / 2);
}

void drawCovid19(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(x + 40, y + 5,  getStr( s_indoor_spread_risk));
  display->drawXbm( x + 0, y + 9, 40, 40, covid_40x40);
  
  uint8_t i = getCovid19Dispersal();
  display->drawString(x + 50, y + 25, getStr(i == 0 ? s_Low : (i == 1 ? s_Medium : s_High)));
  display->drawXbm( x + 90, y + 17, 34, 34, i == 0 ? smile_34x34 : (i == 1 ? smile_warning_34x34 : smile_mask_34x34));
}
