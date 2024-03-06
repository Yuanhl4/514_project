#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0; //what is this?
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
const int vibrationMotorPin = 8;

Adafruit_BME280 bme; // Use I2C interface

unsigned long motorStartCount = 0;

void activateVibrationMotor() {
    digitalWrite(vibrationMotorPin, HIGH);
    delay(1000); // Assuming motor should be active for 1 second
    digitalWrite(vibrationMotorPin, LOW);
    
    // Increment motor start count
    motorStartCount++;
    
    // Convert motor start count to bytes
    /*uint8_t countBytes[4];
    countBytes[0] = (motorStartCount >> 24) & 0xFF;
    countBytes[1] = (motorStartCount >> 16) & 0xFF;
    countBytes[2] = (motorStartCount >> 8) & 0xFF;
    countBytes[3] = motorStartCount & 0xFF;*/

    // Check if a device is connected before trying to send
    if (deviceConnected) {
        // Assuming motorStartCount is the variable tracking the number of motor starts
        String motorStartCountStr = String(motorStartCount);
        pCharacteristic->setValue(motorStartCountStr.c_str());
        //pCharacteristic->setValue(countBytes, sizeof(countBytes));
        pCharacteristic->notify();
        Serial.print("Motor start count notified: ");
        Serial.println(motorStartCountStr);
    }
}


// Update these constants with the actual GPIO pins used for SDA and SCL if they are not the default pins
const int i2cSDA = 21;
const int i2cSCL = 22;

float lastTemperature = 0.0;
float lastHumidity = 0.0;
float tempThreshold = 3.0; // Temperature change threshold to activate the vibration motor
float humidityThreshold = 3.0;

// Define constants for DSP
const int bufferSize = 10; // Size of the buffer for storing sensor readings
float sensorBuffer[bufferSize]; // Buffer to store sensor readings
int bufferIndex = 0;

// Function to apply a moving average filter to the sensor readings
float movingAverageFilter(float newValue) {
    // Add the new value to the buffer
    sensorBuffer[bufferIndex] = newValue;
    
    // Increment buffer index with wraparound
    bufferIndex = (bufferIndex + 1) % bufferSize;
    
    // Calculate the average of the values in the buffer
    float sum = 0.0;
    for (int i = 0; i < bufferSize; i++) {
        sum += sensorBuffer[i];
    }
    return sum / bufferSize;
}

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "56f9819c-fd94-44d4-9d3c-156a6b86279b"
#define CHARACTERISTIC_UUID "0900f701-d655-4987-ab06-ff880acdb3c7"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here

const float alpha = 0.1; // Adjust this value for filtering strength (0.0 to 1.0)

void processSensorData() {
    // Read sensor data
    float currentTemperature = bme.readTemperature();
    
    // Apply DSP algorithm (e.g., moving average filter)
    float filteredTemperature = movingAverageFilter(currentTemperature);
    
    // Perform further processing or analysis based on the filtered data
    // For example, you could trigger an action based on certain conditions:
    if (filteredTemperature > tempThreshold) {
        // Do something...
    }
}

void setup() {
    //Serial.begin(115200);
    Serial.println("Starting BLE work!");

    Serial.begin(115200);
    pinMode(8,OUTPUT); 
    digitalWrite(vibrationMotorPin, HIGH);
    delay(1000); // Vibrate for 1 second
    digitalWrite(vibrationMotorPin, LOW);

    if (!bme.begin(0x76, &Wire)) { // Update 0x76 to your BME280's I2C address if different
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

    pinMode(vibrationMotorPin, OUTPUT);
    digitalWrite(vibrationMotorPin, LOW);

    Wire.begin(i2cSDA, i2cSCL); // Explicitly start I2C with specified pins

    lastTemperature = bme.readTemperature();
    Serial.print("Initial Temperature: ");
    Serial.println(lastTemperature);
    lastHumidity = bme.readHumidity();
    Serial.print("Initial Humidity: ");
    Serial.println(lastHumidity);

    // TODO: name your device to avoid conflictions
    BLEDevice::init("FXXK514");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    processSensorData();

    float currentTemperature = bme.readTemperature();
    float currentHumidity = bme.readHumidity();
    
    bool tempExceeded = (currentTemperature - lastTemperature) > tempThreshold;
    bool humidityExceeded = (currentHumidity - lastHumidity) > humidityThreshold;

    if (tempExceeded || humidityExceeded) {
        Serial.println("Threshold exceeded. Activating vibration motor.");
        activateVibrationMotor();
    }
    
    lastTemperature = currentTemperature;
    lastHumidity = currentHumidity;
    delay(1000);

    
    /*
    float currentTemperature = bme.readTemperature();
    float currentHumidity = bme.readHumidity();

    Serial.print("Current Temperature: ");
    Serial.println(currentTemperature);
    Serial.print("Current Humidity: ");
    Serial.println(currentHumidity);
    
     // Check if the changes exceed the thresholds
    bool tempExceeded = (currentTemperature - lastTemperature) > tempThreshold;
    bool humidityExceeded = (currentHumidity - lastHumidity) > humidityThreshold;

    if (tempExceeded || humidityExceeded) {
        Serial.println("Threshold exceeded. Activating vibration motor.");
        digitalWrite(vibrationMotorPin, HIGH);
        delay(1000); // Vibrate for 1 second
        digitalWrite(vibrationMotorPin, LOW);
    } else {
        Serial.println("Threshold not exceeded.");
    }

    // Update last readings
    lastTemperature = currentTemperature;
    lastHumidity = currentHumidity;

    // Wait a second before reading again
    delay(1000);
    */


    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
        pCharacteristic->setValue("Hello World");
        pCharacteristic->notify();
        //Serial.println("Notify value: Hello World");
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
