# 引脚分配 (Wokwi ESP32 虚拟)
- DHT11 数据脚：GPIO 4
- OLED SDA：GPIO 21
- OLED SCL：GPIO 22
- 继电器/LED：GPIO 5 (高电平触发)

# MQTT 通信协议
- Broker：broker.emqx.io (公共测试用)
- 客户端 ID：esp32-home-001
- 上行主题：`home/env`
  - 格式：`{"temp":25, "humi":60}`
  - QoS：0
- 下行主题：`home/cmd`
  - 格式：`{"led":1}` (1开，0关)
  - QoS：1
