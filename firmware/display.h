#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>


extern Adafruit_SSD1306 display; 

void initDisplay();
void updateDisplay(float t, float h, bool cloudConnected);

#endif