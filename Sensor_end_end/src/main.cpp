#include <Arduino.h>
#include <Wire.h>
#include <SHT31.h>
#include <BluetoothSerial.h>

#define VIBRATION_PIN 12 
#define 


SHT31 sht31;
BluetoothSerial SerialBT;

float previousTemp = 0.0;
float previousHumidity = 0.0;

void setup() {
  Serial.begin(9600);
  SerialBT.begin("d_end"); 
  pinMode(VIBRATION_PIN, OUTPUT);

  Wire.begin(4, 5); // Initialize I2C communication with SDA pin on GPIO 4 and SCL pin on GPIO 5

  if (!sht31.begin()) {
    Serial.println("Couldn't find SHT31 sensor!");
    while (1) delay(1);
  }
}

void loop() {
  delay(2000);

  float temperature = sht31.readTemperature();
  float humidity = sht31.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from SHT31 sensor!");
    return;
  }

  // Check if temperature change is significant
  if (abs(temperature - previousTemp) >= 6.0 && abs(humidity - previousHumidity) >= 5.0) {
    digitalWrite(VIBRATION_PIN, HIGH); // Start vibration motor
    delay(1000); // Vibration duration
    digitalWrite(VIBRATION_PIN, LOW); // Stop vibration motor

    // Send signal to another ESP32 through Bluetooth
    SerialBT.println("Oral Breath detected");
  }

  previousTemp = temperature;
  previousHumidity = humidity;
}
