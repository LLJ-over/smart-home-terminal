#include <WiFi.h>
#include <PubSubClient.h>
#include "display.h" 
#include "sensor.h"   

// --- 硬件引脚 ---
#define LED_PIN 5

const char* ssid = "Wokwi-GUEST";
const char* mqtt_server = "broker.emqx.io";
const char* topic_env = "home/env/ll_test";
const char* topic_cmd = "home/cmd/ll_test"; 

WiFiClient espClient;
PubSubClient client(espClient);

// 接收回调
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  if (message == "ON" || message == "{\"led\":1}") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("指令: ON -> LED亮");
  } else if (message == "OFF" || message == "{\"led\":0}") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("指令: OFF -> LED灭");
  }
}

// 断线重连机制
void reconnect() {
  while (!client.connected()) {
    Serial.print("尝试连接MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("成功!");
      client.subscribe(topic_cmd); 
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // 调用拆出去的初始化函数
  initSensor();
  initDisplay();

  // 连WiFi
  WiFi.begin(ssid);
  Serial.print("正在连接WiFi");
  unsigned long wifiStartTime=millis();

  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");

    if(millis()-wifiStartTime>15000){
      Serial.println("\n WiFi连接超时！进入离线模式...");
      break;
    }
  }

  if(WiFi.status()==WL_CONNECTED){
    Serial.println("\n WiFi连接成功!");
  }
  else{
    // 这里的 display 变量是在 display.h 里 extern 借过来的
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("WiFi Fail!");
    display.println("Running Offline...");
    display.display();
  }
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }

  static unsigned long lastTime = 0;
  if (millis() - lastTime > 2000) {
    lastTime = millis();
    
    // 调用拆出去的传感器函数获取数据
    float h = getHumidity();
    float t = getTemperature();

    if (!isnan(h) && !isnan(t)) {
      // 调用拆出去的显示函数
      updateDisplay(t, h, client.connected());

      // 发送到云端（这部分还留在主函数里）
      String payload = "{\"temp\":" + String(t) + ",\"humi\":" + String(h) + "}";
      client.publish(topic_env, payload.c_str());
    }
  }
}