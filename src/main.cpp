#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)
const int LOWER_TEMP = 21;
const int UPPER_TEMP = 29;
const int LOWER_HUM = 60;
const int UPPER_HUM = 80;

int temp = -1;
int humidity = -1;

Adafruit_BME680 bme; // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

enum State {
  IDLE,
  WATERING,
  WARNING_TEMP,
  WARNING_HUM
};

// function headers
void setupBME();
void checkBME();
bool tempInRange();
bool humidityInRange();

State greenhouse = IDLE;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  setupBME(); 
}

void loop() {
  //checkBME();
  Serial.println("hello world");
  switch (greenhouse) {
    case IDLE: 
      checkBME();
      break;
    case WATERING:
      Serial.println("watering");
      break;
    case WARNING_TEMP:
      Serial.println("Warning: temperature not in range. Please adjust accordingly.");
      Serial.println(String("Temperature: ") + temp + "%");
      delay(1000);
      greenhouse = IDLE;
      break;
    case WARNING_HUM:
      Serial.println("Warning: humidity not in range. Please adjust accordingly.");
      Serial.println(String("Humidity: ") + humidity + "%");
      delay(1000);
      greenhouse = IDLE;
      break;
    default:
      Serial.println("default case");
  }
}

void checkBME() {
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
 // Serial.print("Temperature = ");
  //Serial.print(bme.temperature);
 // Serial.println(" *C");
  temp = bme.temperature;
  if (!tempInRange()) {
    greenhouse = WARNING_TEMP;
  }

  humidity = bme.humidity;
  if (!humidityInRange()) {
    greenhouse = WARNING_HUM;
  }

  Serial.println(humidity);
  delay(3000);
}

bool tempInRange() {
  if ((temp >= LOWER_TEMP) && (temp <= UPPER_TEMP)) {
    Serial.println("temp in range");
    return true;
  } else {
    return false;
  }
}

bool humidityInRange() {
  if ((temp >= LOWER_HUM) && (temp <= UPPER_HUM)) {
    Serial.println("humidity in range");
    return true;
  } else {
    return false;
  }
}




void setupBME() {
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}