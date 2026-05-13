#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 33       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT12   // DHT 11

DHT dht(DHTPIN, DHTTYPE); // Init DHT sensor

float h, t; // Variables to hold humidity and temperature values
float hic;  // Variable to hold heat index value

void setup() {
  Serial.begin(115200); // Init serial
  Serial.println("Init");

  dht.begin(); // Start DHT sensor


}

void loop() {
  delay(2000); // Wait a few seconds between measurements. 
  h = dht.readHumidity();
  t = dht.readTemperature();

  if(isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.println();

  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);
}
