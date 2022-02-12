#include <OLEDDisplayUi.h>
#include "Tools.h"
#include "Sensor.h"

void drawLineChart(OLEDDisplay *display, int16_t x, int16_t y) {
  //Calculate min, max
  int16_t min10 = 32767;
  int16_t max10 = -32768;
  for (uint8_t i = 0; i < TEMP_HIST_SIZE; i++) {
    int16_t d = pSensor->getHist(i);
    //Serial.println( "Data: " + String(i) + "-" + String(d) + ":" + String(Sensor::int2Float( d)));
    if (d == NO_VALUE_INT)  //skip empty values
      continue;
    if ( d > max10)
      max10 = d;
    if ( d < min10)
      min10 = d;
  }
  //Serial.println( "Min-Max: " + String(min10) + "-" + String(max10));
  min10 = floor((float)min10/10.0);
  max10 = ceil((float)max10/10.0);
  if (min10 == max10)
    max10++;
  min10 *= 10;
  max10 *= 10;

  float scale = 27.0 / (float)(max10-min10);
  //Serial.println("-----");  
  //Serial.println( "Min-Max-Scale: " + String(min10) + "-" + String(max10) + "-" + String(scale));
  
  // Plot temperature graph
  int16_t x1 = 23;
  int16_t y1 = 36;
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(display->getWidth() + x, 20 + y, Sensor::strTempUnit());

  // Horizontal axis
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawHorizontalLine(x1 + x, y1 + y, 96);
  for (uint8_t i=0; i<=90; i+=30) {
    int16_t mark = x1+i;
    display->drawLine( mark + x, y1 + y, mark + x, y1+2 + y);
    display->drawString( mark + x, y1+1 + y, i == 90 ? getStr(s_now) : String(-90+i) + "m");
  }
    
  // Vertical axis
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawLine( x1 + x, y1 + y, x1 + x, y1-30+y);
  int16_t step = ceil((float)(max10-min10)/3.0/10.0)*10;
  if (step < 10)
    step = 10;  //Minimal value

  //Serial.println( String( min10) + "~" + String( max10) + "~" + String( scale,2) + "=" + String( step));
  for (int16_t i=min10; i<=max10; i+=step) {
    int16_t mark = round((float)(i - min10) * scale);
    display->drawLine( x1 + x, y1 - mark + y, x1-2 + x, y1 - mark + y);
    display->drawString( x1-4 + x, y1 - mark - 7 + y, String((float)i/10.0, 0));
  }

  int prev = NO_VALUE_INT;
  for (uint8_t i = 0; i < TEMP_HIST_SIZE; i++)
    if ( pSensor->getHist(i) != NO_VALUE_INT) {
      //Serial.println( String(i) + "-" + String(pSensor->getHist(i)) + "-" + String(convertFtoC(pSensor->getHist(i))));
      int16_t d = round((float)(pSensor->getHist(i) - min10) * scale);
      //Serial.println( String( i) + "-" + String( pSensor->getHist(i)) + "=" + String(d));
      if (prev == NO_VALUE_INT)
        display->setPixel( i+x1 + x, y1 - d + y);
      else
        display->drawLine( i+x1 + x - 1, y1 - prev + y, i+x1 + x, y1 - d + y);
      prev = d;
    } else
      prev = NO_VALUE_INT;
}

void drawTemperatureChart(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawLineChart( display, x, y);
}
