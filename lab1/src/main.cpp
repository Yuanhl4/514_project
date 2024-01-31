#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SSD1306_I2C_ADDRESS 0x3C
#define LIGHT_SENSOR_PIN A0
#define SERVO_PIN 1
#define PULSE_SENSOR_PIN 0
#define THRESHOLD 550

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BME280 bme;
Servo servo;

unsigned long lastHeartBeatTime;
int heartRate;

void setup() {
    Serial.begin(9600);
    while (!Serial); // Wait for Serial to be ready
    Serial.println(F("Initializing..."));

    Wire.begin();

    if (!bme.begin(0x76)) {
        Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
        while (1);
    }

    if(!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }

    display.display();
    delay(2000); // Pause for 2 seconds
    display.clearDisplay();

    pinMode(LIGHT_SENSOR_PIN, INPUT);
    pinMode(PULSE_SENSOR_PIN, INPUT);
    servo.attach(SERVO_PIN);

    lastHeartBeatTime = millis();

    Serial.println(F("Initialization complete. Starting loop..."));
}

void loop() {
    Serial.println(F("Reading temperature, humidity, light level, and heart rate..."));

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    int pulseValue = analogRead(PULSE_SENSOR_PIN);

    if (pulseValue > THRESHOLD) {
        heartRate = 60000 / (millis() - lastHeartBeatTime);
        lastHeartBeatTime = millis();
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.print(F("Temp: "));
    display.print(temperature);
    display.print(F(" C"));

    display.setCursor(0, 10);
    display.print(F("Hum: "));
    display.print(humidity);
    display.print(F("%"));

    display.setCursor(0, 20);
    display.print(F("Heart rate: "));
    display.print(heartRate);
    display.print(F(" BPM"));

    display.display();

    Serial.println(F("Checking conditions for servo control..."));
    if (temperature > 30 && humidity > 40 && heartRate > 60) {
        servo.write(180);
        Serial.println(F("Servo rotating."));
    } else {
        servo.write(90);
        Serial.println(F("Servo stopped."));
    }

    Serial.println(F("Checking ambient light for system on/off..."));
    if (lightValue > 500) {
        display.println(F("It is a hot day, right?"));
        Serial.println(F("It is a hot day, right?"));
    } else {
        display.println(F("It is a hot night, right?"));
        Serial.println(F("It is a hot night, right?"));
    }

    display.display();
    delay(2000);
}