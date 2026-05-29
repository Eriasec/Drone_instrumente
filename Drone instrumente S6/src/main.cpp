#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include <Adafruit_BMP280.h>
#include <BluetoothSerial.h>

#define DHTPIN 33       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT12   // DHT 12

uint32_t getAbsoluteHumidity(float temperature, float humidity);

DHT dht(DHTPIN, DHTTYPE); // Init DHT sensor
Adafruit_SGP30 sgp;
// Adafruit_BMP280 bmp; // I2C
BluetoothSerial SerialBT;

float h, t; // Variables to hold humidity and temperature values
float hic;  // Variable to hold heat index value
int counter = 0;
const char* pin = "1234"; // Bluetooth pairing pin

void setup() {
  Serial.begin(115200); // Init serial
  Serial.println("Init");
  
  // _____ Init Temperature/Humidity sensor _____ //
  dht.begin(); // Start DHT sensor

  // _____ Init Air Quality sensor _____ //
  if (! sgp.begin()){
    Serial.println("Sensor not found :(");
    while (1) delay(10);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  // // _____ Init Pressure sensor _____ //
  // if (!bmp.begin()) {
  //   Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
  //                     "try a different address!"));
  //   while (1) delay(10);
  // }

  // _____ Init Bluetooth _____ //
  SerialBT.setPin(pin);                   // Set Bluetooth pairing pin
  SerialBT.begin("ESP32_METEO_STATION");  // Bluetooth device name
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

  SerialBT.print(F("H"));
  SerialBT.print(h);
  SerialBT.print(F("T"));
  SerialBT.print(t);
  SerialBT.print("\n");

  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);

  // If you have a temperature / humidity sensor, you can set the absolute humidity to enable the humditiy compensation for the air quality signals
  sgp.setHumidity(getAbsoluteHumidity(t, h));

  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");

  if (! sgp.IAQmeasureRaw()) {
    Serial.println("Raw Measurement failed");
    return;
  }
  Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
  Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");

  delay(1000);

  counter++;
  if (counter == 30) {
    counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
      Serial.println("Failed to get baseline readings");
      return;
    }
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
  }
}



/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}
