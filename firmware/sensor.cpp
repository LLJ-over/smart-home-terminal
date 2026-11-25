#include "sensor.h"

DHT dht(DHTPIN, DHTTYPE);

void initSensor() {
  dht.begin();
}

float getTemperature() {
  return dht.readTemperature();
}

float getHumidity() {
  return dht.readHumidity();
}