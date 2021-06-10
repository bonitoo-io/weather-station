#include <OLEDDisplayUi.h>
#include "Tools.h"

extern int tempHistory[90];

int convertCtoF(int c) {
  return round(((float)(c/10) * 1.8 + 32) * 10);
}

void drawLineChart(OLEDDisplay *display, const String& unit, bool convert2F, int data[], unsigned int size, int16_t x, int16_t y) {
  //Caclculate min, max and number of samples
  int min10 = 32767;
  int max10 = -32768;
  for (unsigned int i = 0; i < size; i++) {
    int d = data[i];
    if (d == 0xffff)  //skip empty values
      continue;
    if (convert2F)
      d = convertCtoF(d);
    if ( d > max10)
      max10 = d;
    if ( d < min10)
      min10 = d;
  }
  min10 = floor((float)min10/10);
  max10 = ceil((float)max10/10);
  if (min10 == max10)
    max10++;
  min10 *= 10;
  max10 *= 10;

  float scale = 30.0 / (float)(max10-min10);

  // Plot temperature graph
  int x1 = 23;
  int y1 = 39;
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(display->getWidth() + x, 20 + y, unit);

  // Horizontal axis
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawHorizontalLine(x1 + x, y1 + y, 96);
  for (int i=0; i<=90; i+=30) {
    int mark = x1+i;
    display->drawLine( mark + x, y1 + y, mark + x, y1+2 + y);
    display->drawString( mark + x, y1+1 + y, String(-90+i) + "m");
  }
    
  // Vertical axis
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawLine( x1 + x, y1 + y, x1 + x, y1-30+y);
  int step = ceil((float)(max10-min10)/3/10)*10;
  if (step < 10)
    step = 10;  //Minimal value

  //Serial.println( String( min10) + "~" + String( max10) + "~" + String( scale,2) + "=" + String( step));
  for (int i=min10; i<=max10; i+=step) {
    int mark = round((float)(i - min10) * scale);
    display->drawLine( x1 + x, y1 - mark + y, x1-2 + x, y1 - mark + y);
    display->drawString( x1-4 + x, y1 - mark - 7 + y, String((float)i/10, 0));
  }

  for (unsigned int i = 0; i < size; i++)
    if ( data[i] != 0xffff) {
      int d = convert2F ? convertCtoF( data[i]) : data[i];
      d = round((float)(d - min10) * scale);
      //Serial.println( String( i) + "-" + String( data[i]) + "=" + String(d));
      display->setPixel( i+x1 + x, y1 - d + y);
    }
}

void drawTemperatureChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawLineChart( display, strTempUnit(), !conf.useMetric, tempHistory, sizeof(tempHistory) / sizeof(tempHistory[0]), x, y);
}
