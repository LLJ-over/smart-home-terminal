#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// --- 硬件引脚 ---
#define DHTPIN 4
#define DHTTYPE DHT11
#define LED_PIN 5
DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// --- 网络配置 ---
const char* ssid = "Wokwi-GUEST";
const char* mqtt_server = "broker.emqx.io";
const char* topic_env = "home/env/ll_test"; // 记得换成你自己的主题
const char* topic_cmd = "home/cmd/ll_test"; // 记得换成你自己的主题

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
      client.subscribe(topic_cmd); // 订阅控制指令
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  dht.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  // 连WiFi
  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // 非阻塞式定时器：每2秒采集并刷新
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 2000) {
    lastTime = millis();
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      // 1. 显示在OLED
      display.clearDisplay();
      display.setCursor(0, 0); display.print("Temp: "); display.print(t); display.println(" C");
      display.setCursor(0, 20); display.print("Humi: "); display.print(h); display.println(" %");
      display.setCursor(0, 40); display.println(client.connected() ? "Cloud: Online" : "Cloud: Offline");
      display.display();

      // 2. 发送到云端
      String payload = "{\"temp\":" + String(t) + ",\"humi\":" + String(h) + "}";
      client.publish(topic_env, payload.c_str());
    }
  }
}


