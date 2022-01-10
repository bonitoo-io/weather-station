#include <OLEDDisplayUi.h>
#include "Tools.h"
#include "WeatherStation.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

bool updateClock( bool firstStart, int utc_offset, const String &ntp) {
  //Convert ntp comma separated list to array
  char ntpbuff[50];
  char *ntparr[3];
  strncpy(ntpbuff, ntp.c_str(), sizeof( ntpbuff));
  ntparr[0] = strtok( ntpbuff, ",");
  for (unsigned int i = 1; i < sizeof( ntparr)/sizeof(char*); i++)
    ntparr[i] = strtok( NULL, ",");

  //Set TZ and NTP
  configTime( utc_offset, 0, ntparr[0], ntparr[1], ntparr[2]);
  if (firstStart) {
    // Wait till time is synced
    Serial.print(F("Syncing time"));
    int i = 0;
    while (time(nullptr) < 1000000000ul && i < 40) {
      Serial.print(F("."));
      delay(500);
      i++;
    }
    Serial.println();
  
    // Show time
    time_t now = time(nullptr);
    Serial.print(F("Synchronized time: "));
    Serial.println(ctime(&now));
  }
  return time(nullptr) > 1000000000ul;
}


void drawDateTimeAnalog(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int clockCenterX=30+x;
  int clockCenterY=28+y;
  const int clockSize=20;

  time_t now = time(nullptr);
  struct tm* t;
  t = localtime(&now);

  // Draw marks for hours
  for (int i=0; i<12; i++) {
    float f = ((i * 30) + 270) * 0.0175;
    display->drawLine(clockSize*cos(f)+clockCenterX, clockSize*sin(f)+clockCenterY, (clockSize-4+(i%3==0?0:3))*cos(f)+clockCenterX, (clockSize-4+(i%3==0?0:3))*sin(f)+clockCenterY);
  }

  //draw seconds
  float s = ((t->tm_sec*6)+270)*0.0175;
  display->drawLine(clockSize*cos(s)+clockCenterX, clockSize*sin(s)+clockCenterY, cos(s)+clockCenterX, sin(s)+clockCenterY);

  //Draw minutes
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int m=(t->tm_min*6)+270;

  x1=(clockSize-3)*cos(m*0.0175);
  y1=(clockSize-3)*sin(m*0.0175);
  x2=cos(m*0.0175);
  y2=sin(m*0.0175);
  x3=10*cos((m+8)*0.0175);
  y3=10*sin((m+8)*0.0175);
  x4=10*cos((m-8)*0.0175);
  y4=10*sin((m-8)*0.0175);

  display->drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  display->drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  display->drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  display->drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  //draw hour
  int h=(t->tm_hour*30)+(t->tm_min/2)+270;

  x1=(clockSize-7)*cos(h*0.0175);
  y1=(clockSize-7)*sin(h*0.0175);
  x2=cos(h*0.0175);
  y2=sin(h*0.0175);
  x3=8*cos((h+12)*0.0175);
  y3=8*sin((h+12)*0.0175);
  x4=8*cos((h-12)*0.0175);
  y4=8*sin((h-12)*0.0175);

  display->drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  display->drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  display->drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  display->drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  //draw date
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 10 + y, strDate(now, false));
}

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t now = time(nullptr);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);

  display->drawString(64 + x, 8 + y, strDate(now, true));

  display->setFont(DSEG7_Classic_Bold_21);
  display->drawString(64 + x - (station.getRegionalSettings()->use24Hours ? 0 : 4), 20 + y, strTime(now, false));

  if (!station.getRegionalSettings()->use24Hours) {
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->setFont(ArialMT_Plain_10);
    display->drawString(display->getWidth() + x, 18 + y, strTimeSuffix(now));
  }
}
