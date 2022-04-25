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

//No night mode - return 0
//Night mode - return number of seconds till end of minute
uint8_t isNightMode( DisplaySettings *pDisplaySettings) {
  if (pDisplaySettings->nightModeBegin == pDisplaySettings->nightModeEnd)
    return 0; //no night mode

  //get actual time
  time_t now = time(nullptr);
  struct tm *timeInfo = localtime(&now);
  int16_t itm = (timeInfo->tm_hour * 100) + timeInfo->tm_min;
  //Serial.printf_P(PSTR("Night itm %d, begin %d, end %d\n"), itm,  pDisplaySettings->nightModeBegin, pDisplaySettings->nightModeEnd);
  if( (pDisplaySettings->nightModeBegin > pDisplaySettings->nightModeEnd && (itm >= pDisplaySettings->nightModeBegin || itm < pDisplaySettings->nightModeEnd))
      || (pDisplaySettings->nightModeBegin < pDisplaySettings->nightModeEnd && (itm >= pDisplaySettings->nightModeBegin && itm < pDisplaySettings->nightModeEnd)) ) {
    return (60 - timeInfo->tm_sec) + 1; //return number of seconds till end of the minute, cannot be 0!
  }
  return 0;  //no night mode
}
