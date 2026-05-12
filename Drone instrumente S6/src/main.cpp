#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 33       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

DHT_Unified dht(DHTPIN, DHTTYPE); // Init DHT sensor

void setup() {
  Serial.begin(115200); // Init serial
  dht.begin(); // Start DHT sensor
}

void loop() {
  
}