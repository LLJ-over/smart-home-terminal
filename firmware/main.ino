#include <WiFi.h>
#include "display.h"
#include "sensor.h"
#include "net.h"

#define LED_PIN 5

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  initSensor();
  initDisplay();
  
  setupWiFi(); // 这里只调用连WiFi
  
  if(WiFi.status() != WL_CONNECTED){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("WiFi Fail!");
    display.println("Running Offline...");
    display.display();
  }
}

void loop() {
  mqttLoop(); // 网络维持全权交给 net.cpp

  static unsigned long lastTime = 0;
  if (millis() - lastTime > 2000) {
    lastTime = millis();
    
    float h = getHumidity();
    float t = getTemperature();

    if (!isnan(h) && !isnan(t)) {
      updateDisplay(t, h, client.connected()); // client是在net.h里extern过来的
      publishEnvData(t, h); // 发布数据
    }
  }
}