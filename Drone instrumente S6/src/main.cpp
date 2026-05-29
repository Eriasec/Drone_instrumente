#include <Arduino.h>
#include <Adafruit_Sensor.h>

// Parametres
#define ENABLE_DEBUG
#define ENABLE_DHT12
#define ENABLE_SGP30
//#define ENABLE_BMP280
#define ENABLE_BLUETOOTH

#ifdef ENABLE_DHT12
  #include <Wire.h>
  #include <DHT.h>
  #include <DHT_U.h>
  #define DHTPIN 33       // Digital pin connected to the DHT sensor
  #define DHTTYPE DHT12   // DHT 12
  DHT dht(DHTPIN, DHTTYPE); // Init DHT sensor
#endif

#ifdef ENABLE_SGP30
  #include <Wire.h>
  #include <Adafruit_SGP30.h>
  Adafruit_SGP30 sgp;
#endif

#ifdef ENABLE_BMP280
  #include <Wire.h>
  #include <Adafruit_BMP280.h>
  Adafruit_BMP280 bmp; // I2C
#endif

#ifdef ENABLE_BLUETOOTH
  #include "BluetoothSerial.h"
  BluetoothSerial SerialBT;
#endif

uint32_t getAbsoluteHumidity(float temperature, float humidity);

float h, t; // Variables to hold humidity and temperature values
float hic;  // Variable to hold heat index value
int counter = 0;
const char* pin = "1234"; // Bluetooth pairing pin

void setup() {
  #ifdef ENABLE_DEBUG     // Init Serial for debugging
    Serial.begin(115200);
    Serial.println("Init");
  #endif

  #ifdef ENABLE_DHT12     //  Init Temperature/Humidity sensor
    dht.begin();
  #endif

  #ifdef ENABLE_SGP30     // Init Air Quality sensor
    if (! sgp.begin()){
      #ifdef ENABLE_DEBUG
        Serial.println("Sensor not found :(");
      #endif
      while (1) delay(10);
    }
    #ifdef ENABLE_DEBUG
      Serial.print("Found SGP30 serial #");
      Serial.print(sgp.serialnumber[0], HEX);
      Serial.print(sgp.serialnumber[1], HEX);
      Serial.println(sgp.serialnumber[2], HEX);
    #endif
  #endif

  #ifdef ENABLE_BMP280    //Init Pressure sensor
    if (!bmp.begin()) {
      #ifdef ENABLE_DEBUG
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                        "try a different address!"));
      #endif
      while (1) delay(10);
    }
  #endif

  #ifdef ENABLE_BLUETOOTH // Init Bluetooth
    SerialBT.begin("ESP32_METEO_STATION");  // Bluetooth device name
  #endif
}

void loop() {
  delay(2000); // Wait a few seconds between measurements. 
  #ifdef ENABLE_DHT12     // DHT12 readings
    h = dht.readHumidity();
    t = dht.readTemperature();

    if(isnan(h) || isnan(t)) {
      #ifdef ENABLE_DEBUG
        Serial.println("Failed to read from DHT sensor!");
      #endif
      return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    hic = dht.computeHeatIndex(t, h, false);

    #ifdef ENABLE_DEBUG
      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(t);
      Serial.print(F("°C  Heat index: "));
      Serial.print(hic);
      Serial.print(F("°C "));
      Serial.println();
    #endif

    #ifdef ENABLE_BLUETOOTH
      SerialBT.print(F("HUM"));
      SerialBT.print(h);
      SerialBT.print(F(" TEMP"));
      SerialBT.print(t);
      SerialBT.print(F(" INDEX"));
      SerialBT.print(hic);
      #ifndef ENABLE_SGP30
        #ifndef ENABLE_BMP280
          SerialBT.print(F("\n"));
        #endif
      #endif
    #endif
    
    #ifdef ENABLE_SGP30 // Set absolute humidity for SGP30 compensation
      sgp.setHumidity(getAbsoluteHumidity(t, h));
    #endif
  #endif

  #ifdef ENABLE_SGP30     // SGP30 readings
    if (! sgp.IAQmeasure()) {
      #ifdef ENABLE_DEBUG
        Serial.println("Measurement failed");
      #endif
      return;
    }
    #ifdef ENABLE_DEBUG
      Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
      Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");
    #endif

    if (! sgp.IAQmeasureRaw()) {
      #ifdef ENABLE_DEBUG
        Serial.println("Raw Measurement failed");
      #endif
      return;
    }
    #ifdef ENABLE_DEBUG
      Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
      Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");
    #endif

    counter++;
    if (counter == 30) {
      counter = 0;

      uint16_t TVOC_base, eCO2_base;
      if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
        #ifdef ENABLE_DEBUG
          Serial.println("Failed to get baseline readings");
        #endif
        return;
      }
      #ifdef ENABLE_DEBUG
        Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
        Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
      #endif
    }

    #ifdef ENABLE_BLUETOOTH
      SerialBT.print(F(" TVOC"));
      SerialBT.print(sgp.TVOC);
      SerialBT.print(F(" CO2"));
      SerialBT.print(sgp.eCO2);
      SerialBT.print(F(" RAWH2"));
      SerialBT.print(sgp.rawH2);
      SerialBT.print(F(" RAWET"));
      SerialBT.print(sgp.rawEthanol);
      SerialBT.print(F("\n"));
      #endif
  #endif
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
