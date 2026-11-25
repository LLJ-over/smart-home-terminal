#include "display.h"
#include <Wire.h>

// 真正的实例化放在 cpp 文件里
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void initDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void updateDisplay(float t, float h, bool cloudConnected) {
  display.clearDisplay();
  display.setCursor(0, 0); 
  display.print("Temp: "); display.print(t); display.println(" C");
  display.setCursor(0, 20); 
  display.print("Humi: "); display.print(h); display.println(" %");
  display.setCursor(0, 40); 
  display.println(cloudConnected ? "Cloud: Online" : "Cloud: Offline");
  display.display();
}