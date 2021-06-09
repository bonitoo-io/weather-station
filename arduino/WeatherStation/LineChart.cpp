#include <OLEDDisplayUi.h>
#include "Tools.h"

extern int tempHistory[96];

void drawLineChart(OLEDDisplay *display, int data[], unsigned int size, int16_t x, int16_t y) {
  //Caclculate min, max and number of samples
  int min = 32767;
  int max = -32768;
  int samples = 0;
  for (unsigned int i = 0; i < size; i++) {
    if (data[i] == 0xffff)  //skip empty values
      continue;
    if ( data[i] > max)
      max = data[i];
    if ( data[i] < min)
      min = data[i];
    samples++;
  }
  min = floor(min);
  max = ceil(max);
  if (min == max)
    max++;
  if (min == (max + 1))
    min--;
  
  // Plot temperature graph
  int x1 = 16;
  int y1 = 39;
  int yscale = 2;                         // Points per degree
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 5 + y, strTempUnit());

  // Horizontal axis
  display->drawHorizontalLine(x1 + x, y1 + y, 96);
  for (int i=0; i<=24; i=i+4) {
    int mark = x1+i*4;
    display->drawLine( mark + x, y1 + y, mark + x, y1+2 + y);
    display->drawString( mark + x, y1+1 + y, String(i));
  }
    
  // Vertical axis
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawLine( x1 + x, y1 + y, x1 + x, y1-30+y);
  for (int i=5; i<=20; i=i+5) {
    int mark = y1-(i*yscale-10);
    display->drawLine( x1 + x, mark + y, x1-2 + x, mark + y);
    display->drawString( x1-4 + x, mark - 6 + y, String(i));
  }

  //Data
  samples = 96;
  if (samples > 96)
    samples = 96;
  for (unsigned int i = 0; i < samples; i++) {
    int data = ((sin(float(i)/10) + 1) * 15) + 11;
    display->setPixel( i+x1 + x, y1 - (data-10 + y));
  }
}

void drawTemperatureChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawLineChart( display, tempHistory, sizeof(tempHistory) / sizeof(tempHistory[0]), x, y);
}
