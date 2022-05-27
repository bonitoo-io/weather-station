#include <OLEDDisplayUi.h>
#include "Tools.h"
#include "WeatherStation.h"
#include "WeatherStationFonts.h"

void drawNight(OLEDDisplay *display) {
  time_t now = time(nullptr);
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString( random(85), random(36), strTime(now, true) + strTimeSuffix(now));
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
