#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// ============================================================
//  硬件引脚
// ============================================================
#define DHTPIN    4
#define DHTTYPE   DHT11
#define LED_PIN   5
#define OLED_SDA  21
#define OLED_SCL  22
#define OLED_ADDR 0x3C

// ============================================================
//  配置
// ============================================================
static const char*     WIFI_SSID   = "Wokwi-GUEST";
static const char*     MQTT_SERVER = "broker.emqx.io";
static const uint16_t  MQTT_PORT   = 1883;
static const char*     TOPIC_ENV   = "home/env/ll_test";
static const char*     TOPIC_CMD   = "home/cmd/ll_test";
static const char*     CLIENT_PREFIX = "esp32-home";

// 时间参数（毫秒）
static const unsigned long SAMPLE_INTERVAL_MS = 2000;
static const unsigned long WIFI_TIMEOUT_MS    = 15000;
static const unsigned long MQTT_RETRY_MS      = 5000;

// 缓冲区大小（全部在栈上，不用 heap）
static const size_t MAX_PAYLOAD = 64;
static const size_t MAX_CMD     = 32;
static const size_t MAX_CLIENTID = 32;

// ============================================================
//  对象
// ============================================================
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WiFiClient espClient;
PubSubClient client(espClient);

// ============================================================
//  运行状态
// ============================================================
static unsigned long lastSampleMs  = 0;
static unsigned long lastMqttTryMs = 0;
static bool          ledState      = false;

// ============================================================
//  MQTT 下行回调：定长缓冲，不做 String 拼接
//  改动 2：原来 String message += payload[i] 会无限增长
// ============================================================
void onMessage(char* topic, byte* payload, unsigned int length) {
  char cmd[MAX_CMD];

  size_t n = (length < (MAX_CMD - 1)) ? (size_t)length : (MAX_CMD - 1);
  memcpy(cmd, payload, n);
  cmd[n] = '\0';

  if (length > n) {
    // 超长指令：截断后仍然匹配不上，走 unknown 分支，安全
    Serial.print(F("[CMD] truncated, len="));
    Serial.println(length);
  }

  if (strcmp(cmd, "ON") == 0 || strcmp(cmd, "{\"led\":1}") == 0) {
    ledState = true;
    Serial.println(F("[CMD] ON -> LED 亮"));
  } else if (strcmp(cmd, "OFF") == 0 || strcmp(cmd, "{\"led\":0}") == 0) {
    ledState = false;
    Serial.println(F("[CMD] OFF -> LED 灭"));
  } else {
    Serial.print(F("[CMD] unknown: "));
    Serial.println(cmd);
  }

  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

// ============================================================
//  MQTT 重连：试一次就返回，绝不 while 阻塞
//  改动 3：原来 while(!client.connected()) + delay(2000)
//          会把整个 loop 冻住，OLED 停刷、传感器停采
// ============================================================
bool tryConnectOnce() {
  char clientId[MAX_CLIENTID];
  snprintf(clientId, sizeof(clientId), "%s-%04X",
           CLIENT_PREFIX, (unsigned)(millis() & 0xFFFF));

  if (!client.connect(clientId)) {
    Serial.print(F("[MQTT] connect failed, state="));
    Serial.println(client.state());
    return false;
  }

  Serial.println(F("[MQTT] connected"));
  client.subscribe(TOPIC_CMD, 1);
  return true;
}

// ============================================================
//  本地显示：同样用定长缓冲
// ============================================================
void updateOled(float t, float h) {
  char line[24];

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  snprintf(line, sizeof(line), "Temp: %.1f C", t);
  display.setCursor(0, 0);
  display.print(line);

  snprintf(line, sizeof(line), "Humi: %.1f %%", h);
  display.setCursor(0, 20);
  display.print(line);

  display.setCursor(0, 40);
  display.print(client.connected() ? F("Cloud: Online") : F("Cloud: Offline"));

  display.display();
}

// ============================================================
//  上行发布：snprintf 进栈缓冲，替换 String 拼接
//  改动 1：原来 "{\"temp\":" + String(t) + ... 每次都可能分配堆内存，
//          长时间运行会导致堆碎片
// ============================================================
void publishEnv(float t, float h) {
  char buf[MAX_PAYLOAD];

  int len = snprintf(buf, sizeof(buf),
                     "{\"temp\":%.1f,\"humi\":%.1f}", t, h);

  if (len < 0 || (size_t)len >= sizeof(buf)) {
    // 截断了就丢弃，绝不发送半个 JSON
    Serial.println(F("[PUB] payload too long, dropped"));
    return;
  }

  if (!client.publish(TOPIC_ENV, buf)) {
    Serial.println(F("[PUB] publish failed"));
  }
}

// ============================================================
//  setup：WiFi 带超时，连不上也要进主循环
//  改动 4：原来 while(WiFi.status()!=WL_CONNECTED) delay(500)
//          没有超时，连不上就永远卡死，OLED 全黑
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[OLED] init failed"));
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Booting..."));
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_TIMEOUT_MS) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("[WiFi] connected, ip="));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("[WiFi] timeout - keep running, will retry in background"));
  }

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(onMessage);
  client.setKeepAlive(30);
}

// ============================================================
//  loop：全程非阻塞
// ============================================================
void loop() {
  // --- 网络维护 ---
  if (!client.connected()) {
    if ((millis() - lastMqttTryMs) >= MQTT_RETRY_MS) {
      lastMqttTryMs = millis();
      tryConnectOnce();
    }
  } else {
    client.loop();
  }

  // --- 采集与显示：不管网络状态都必须跑 ---
  if ((millis() - lastSampleMs) >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("[DHT] read failed, retry next cycle"));
      return;
    }

    updateOled(t, h);

    if (client.connected()) {
      publishEnv(t, h);
    }
  }
}
