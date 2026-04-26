#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Adafruit_seesaw.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

const int RELAY_PIN = 2;
const int BUTTON_PIN = 3;

#define SEALEVELPRESSURE_HPA (1013.25)
const int LOWER_TEMP = 21;
const int UPPER_TEMP = 29;
const int LOWER_HUM = 60;
const int UPPER_HUM = 80;

int temp = -1;
int humidity = -1;

const unsigned long DASH_INTERVAL = 60000; // 60,000 ms = 1 minute
const unsigned long HUM_INTERVAL = 80000;
unsigned long lastDashboard = DASH_INTERVAL;
unsigned long lastHumAlert = HUM_INTERVAL;


Adafruit_BME680 bme; // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

// sensor stuff
Adafruit_seesaw soilSensor; // soil moisture sensor object

const int WINDOW_SIZE = 4; // size of moving average window
float moistureReadings[WINDOW_SIZE];

int currentIndex = 0;
float avgMoisture = 0;
bool soilIsWet = false;
bool buttonPushed = false;

enum State {
  IDLE,
  WATERING,
  WARNING_TEMP,
  WARNING_HUM,
};

// function headers
void setupBME();
void checkBME();
bool tempInRange();
bool humidityInRange();
void checkDashboard();
void printDashboard();
void checkHumidityAlert();
void checkSoil();
void checkButton();
void waterPlant();

State greenhouse = IDLE;

void setup() {
  //digitalWrite(RELAY_PIN, LOW);
  Serial.begin(9600);

  //setup relay
  //pinMode(RELAY_PIN, OUTPUT);
 // digitalWrite(RELAY_PIN, LOW);

  //setup button
  //pinMode(BUTTON_PIN, INPUT_PULLUP);

  while (!Serial);
  //Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  setupBME(); 

  // initialize sensor
  if (!soilSensor.begin(0x36)) {
    Serial.println("Could not find seesaw sensor. Check wiring.");
    while (1);
  }

  Serial.print("Seesaw initialized. Version: ");
  Serial.println(soilSensor.getVersion(), HEX);
}

void loop() {
  //checkBME();
  //Serial.println("hello world");
  //digitalWrite(RELAY_PIN, HIGH);
  switch (greenhouse) {
    case IDLE: 
      checkBME();
      checkSoil();
      checkDashboard();
      checkButton();
      break;
    /*case WATERING:
      Serial.println("watering");
      waterPlant();
      delay(100);
      greenhouse = IDLE;
      break;*/
    case WARNING_TEMP:
      Serial.println("Warning: temperature not in range. Please adjust accordingly.");
      Serial.println(String("Temperature: ") + temp + "%");
      delay(1000);
      greenhouse = IDLE;
      break;
    case WARNING_HUM:
      checkHumidityAlert();
      greenhouse = IDLE;
      break;
    default:
      Serial.println("default case");
  }
}

void checkBME() {
  if (!bme.performReading()) {
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
}

void checkButton() {
  buttonPushed = digitalRead(BUTTON_PIN);
  //Serial.println("button checked");

  if (buttonPushed == LOW) {
    Serial.println("button pushed");
    greenhouse = WATERING;
  }
}

bool tempInRange() {
  if ((temp >= LOWER_TEMP) && (temp <= UPPER_TEMP)) {
    //Serial.println("temp in range");
    return true;
  } else {
    return false;
  }
}

bool humidityInRange() {
  if ((humidity >= LOWER_HUM) && (humidity <= UPPER_HUM)) {
    //Serial.println("humidity in range");
    return true;
  } else {
    return false;
  }
}

void checkDashboard() {
  if (millis() - lastDashboard >= DASH_INTERVAL) {
    printDashboard();
    lastDashboard = millis();
  }
}

void checkHumidityAlert() {
  if (millis() - lastHumAlert >= HUM_INTERVAL) {
    Serial.println("Warning: humidity not in range. Please adjust accordingly.");
    Serial.println(String("Humidity: ") + humidity + "%");
    lastHumAlert = millis();
  }
}

void waterPlant() {
    /*for (int i = 0; i < 10; i++) {
    digitalWrite(RELAY_PIN, LOW);  // ON
    delay(200);                    // Short burst
    digitalWrite(RELAY_PIN, HIGH); // OFF
    delay(800);   */
    //}
    // Longer pause
    digitalWrite(RELAY_PIN, HIGH);  // ON
    delay(200);                    // Short burst
    digitalWrite(RELAY_PIN, LOW);
}

void setupBME() {
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void checkSoil() {
  float temperature = soilSensor.getTemp();
  uint16_t moisture = soilSensor.touchRead(0);

  // store reading in array (circular buffer)
  if (currentIndex >= WINDOW_SIZE) {
    currentIndex = 0;
  }

  moistureReadings[currentIndex] = moisture;
  currentIndex++;

  // compute average
  avgMoisture = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) {
    avgMoisture += moistureReadings[i];
  }
  avgMoisture /= WINDOW_SIZE;

  // determine if soil is wet
  if (avgMoisture > 400) {
    soilIsWet = true;
  } else {
    soilIsWet = false;
  }
}


void printDashboard() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  float temp = bme.temperature;
  float humidity = bme.humidity;
  float gas = bme.gas_resistance / 1000.0;

  // update state based on temp
  if (!tempInRange()) {
    greenhouse = WARNING_TEMP;
  } else {
    greenhouse = IDLE;
  }
  

  Serial.println("\n----- GREENHOUSE STATUS -----");

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Soil rating: ");
  Serial.print(soilIsWet ? "Moist " : "Dry");
  Serial.println(String("\nAverage soil: ") + avgMoisture);  

  Serial.print("Gas: ");
  Serial.print(gas);
  Serial.println(" KOhms");

  Serial.print("Pressure: ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Altitude: ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("State: ");
  switch (greenhouse) {
    case IDLE: Serial.println("IDLE"); break;
    case WATERING: Serial.println("WATERING"); break;
    case WARNING_TEMP: Serial.println("WARNING_TEMP"); break;
    case WARNING_HUM: Serial.println("WARNING_HUM"); break;
  }

  Serial.println("-----------------------------\n");
}
