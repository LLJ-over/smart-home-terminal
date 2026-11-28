#include "net.h"

#define LED_PIN 5 

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Wokwi-GUEST";
const char* mqtt_server = "broker.emqx.io";
const char* topic_env = "home/env/ll_test";
const char* topic_cmd = "home/cmd/ll_test"; 

// 回调
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

// 连WiFi（带15秒超时）
void setupWiFi() {
  WiFi.begin(ssid);
  Serial.print("正在连接WiFi");
  unsigned long wifiStartTime = millis();

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
    if(millis() - wifiStartTime > 15000){
      Serial.println("\n WiFi连接超时！进入离线模式...");
      break;
    }
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\n WiFi连接成功!");
  }
  
  // 【非常重要】设置服务器地址！
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); // 顺便把回调也绑了
}

// 【改造】MQTT非阻塞重连（只试一次，不卡死）
void mqttReconnect() {
  Serial.print("尝试连接MQTT...");
  String clientId = "ESP32Client-" + String(random(0xffff), HEX);
  
  if (client.connect(clientId.c_str())) {
    Serial.println("成功!");
    client.subscribe(topic_cmd); 
  } else {
    Serial.println("失败，等待下次重试");
  }
}

// 维持网络状态（每5秒检查一次重连）
void mqttLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      // 用非阻塞定时器来控制重连频率，防止刷屏
      static unsigned long lastReconnectAttempt = 0;
      if (millis() - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = millis();
        mqttReconnect();
      }
    } else {
      client.loop(); // 只有连上了才跑心跳
    }
  }
}

// 发布数据
void publishEnvData(float t, float h) {
  if (client.connected()) {
    String payload = "{\"temp\":" + String(t) + ",\"humi\":" + String(h) + "}";
    client.publish(topic_env, payload.c_str());
  }
}