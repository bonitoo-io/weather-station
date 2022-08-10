#include <OLEDDisplayUi.h>
#include "Tools.h"
#include "WeatherStation.h"
#include "WeatherStationFonts.h"

void drawNight(OLEDDisplay *display, DisplaySettings *pDisplaySettings) {
  time_t now = time(nullptr);
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  const uint8_t *font = pDisplaySettings->nightModeBigFont?ArialMT_Plain_24:ArialMT_Plain_10;
  display->setFont(font);
  String time = strTime(now, true) + strTimeSuffix(now);
  uint16_t tw = display->getStringWidth(time);
  uint8_t fs = pgm_read_byte(font+1);
  display->drawString( random(128-tw), random(51-fs-1), time);
  display->display();
}


bool isNightMode( DisplaySettings *pDisplaySettings) {
  if (pDisplaySettings->nightModeBegin == pDisplaySettings->nightModeEnd)
    return false; //no night mode

  //get actual time
  time_t now = time(nullptr);
  struct tm *timeInfo = localtime(&now);
  int16_t itm = (timeInfo->tm_hour * 100) + timeInfo->tm_min;
  if( (pDisplaySettings->nightModeBegin > pDisplaySettings->nightModeEnd && (itm >= pDisplaySettings->nightModeBegin || itm < pDisplaySettings->nightModeEnd))
      || (pDisplaySettings->nightModeBegin < pDisplaySettings->nightModeEnd && (itm >= pDisplaySettings->nightModeBegin && itm < pDisplaySettings->nightModeEnd)) ) {
    return true; //return number of seconds till end of the minute, cannot be 0!
  }
  return false;  //no night mode
}
