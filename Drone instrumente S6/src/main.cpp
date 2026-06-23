#include <Arduino.h>
#include <Adafruit_Sensor.h>

// _____ Parametres _____
#define ENABLE_DEBUG
#define ENABLE_DHT12
#define ENABLE_SGP30
#define ENABLE_BMP280
// #define ENABLE_BLUETOOTH
#define ENABLE_WIFI

// _____ Declarations _____
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

  float alt;
  float press;
#endif

#ifdef ENABLE_BLUETOOTH
  #include "BluetoothSerial.h"

  BluetoothSerial SerialBT;
#endif

#ifdef ENABLE_WIFI
  #include "WiFi.h"
  #include "AsyncUDP.h"

  const char *ssid = "Bebop2-070980";
  const char *password = "";

  AsyncUDP udp;
#endif

uint32_t getAbsoluteHumidity(float temperature, float humidity);

float h, t; // Variables to hold humidity and temperature values
float hic;  // Variable to hold heat index value
double initialPressure; // Variable to hold initial pressure value for altitude calculation
int counter = 0;
char message[200];
int offset = 0;
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
    if (!bmp.begin(0x76)) {
      #ifdef ENABLE_DEBUG
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
        Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
      #endif
      while (1) delay(10);
    }
    initialPressure = bmp.readPressure()/100; // Read initial pressure for altitude calculation (hPa)
  #endif

  #ifdef ENABLE_BLUETOOTH // Init Bluetooth
    SerialBT.begin("ESP32_METEO_STATION");  // Bluetooth device name
  #endif

  #ifdef ENABLE_WIFI // Init UDP Client
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi Failed");
      while (1) {
        delay(1000);
      }
    }
    if (udp.connect(IPAddress(192, 168, 42, 100), 1234)) {
      Serial.println("UDP connected");
    //Send unicast
    udp.print("Hello Server!");
  }
  #endif
}

void loop() {
  delay(500); // Wait a few seconds between measurements. 
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

    offset += sprintf(message + offset, "HUM:%f;TEMP:%f;INDEX:%f;", h, t, hic);
    
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

    offset += sprintf(message + offset, "TVOC:%u;CO2:%u;RAWH2:%u;RAWET:%u;", sgp.TVOC, sgp.eCO2, sgp.rawH2, sgp.rawEthanol);
  #endif

  #ifdef ENABLE_BMP280     // BMP280 readings
    press  = bmp.readPressure() / 100;
    alt = bmp.readAltitude(initialPressure);
    #ifdef ENABLE_DEBUG
      Serial.print(F("Pressure = "));
      Serial.print(press);
      Serial.println(" Pa");

      Serial.print(F("Approx. Altitude = "));
      Serial.print(alt); // Adjusted to your local forecasted sea level pressure
      Serial.println(" m");
    #endif

    offset += sprintf(message + offset, "PRES:%f;ALT:%f;", press, alt);
  #endif

  #ifdef ENABLE_BLUETOOTH   // End of frame
    SerialBT.print(message);
  #endif
  
  offset += sprintf(message + offset, "\n");
  Serial.println(message);
  offset = 0;

  #ifdef ENABLE_WIFI
    udp.broadcastTo(message, 52814);
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
