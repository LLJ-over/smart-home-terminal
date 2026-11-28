#ifndef NET_H
#define NET_H

#include <WiFi.h>
#include <PubSubClient.h>

extern WiFiClient espClient;
extern PubSubClient client;

void setupWiFi();
void mqttReconnect();
void mqttLoop();
void publishEnvData(float t, float h);

#endif