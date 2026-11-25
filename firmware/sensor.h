#ifndef SENSOR_H
#define SENSOR_H

#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22

extern DHT dht;

void initSensor();
float getTemperature();
float getHumidity();

#endif