#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "BLEDevice.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SwitecX25.h> // Include SwitecX25 library

// OLED display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Initialize Adafruit SSD1306 display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*
// Define motor steps and pins for SwitecX25
#define STEPS_PER_REVOLUTION 315// Adjust based on your motor's specs
#define MOTOR_PIN1 1  // Adjust according to your setup
#define MOTOR_PIN2 2  // Adjust according to your setup
#define MOTOR_PIN3 4  // Adjust according to your setup
#define MOTOR_PIN4 3  // Adjust according to your setup
SwitecX25 motor(STEPS_PER_REVOLUTION, MOTOR_PIN1, MOTOR_PIN2, MOTOR_PIN3, MOTOR_PIN4);
*/

#define STEPS 945 // standard X25.168 ranges from 0 to 315 degrees, at 1/3 degree increments

// For motors connected to ESP32S3 D2, D3, D1, D0
SwitecX25 motor(STEPS, D0, D1, D2, D3);

// The remote service and characteristic we wish to connect to
static BLEUUID serviceUUID("56f9819c-fd94-44d4-9d3c-156a6b86279b");
static BLEUUID charUUID("0900f701-d655-4987-ab06-ff880acdb3c7");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

uint32_t cumulativeMotorStarts = 0;

int desiredMotorPosition = 0;

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String valueStr = String((char*)pData);
    uint32_t valueCount = valueStr.toInt();
    cumulativeMotorStarts = valueCount;

    if (cumulativeMotorStarts != 0) {
        Serial.print("Cumulative Motor Starts: ");
        Serial.println(cumulativeMotorStarts);
        
        // Update OLED display
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("The Total Times:");
        display.println(cumulativeMotorStarts);
        display.display();
        delay(1000);
    }
  //cumulativeMotorStarts = cumulativeMotorStarts;
    // Rotate SwitecX25 motor based on valueCount
    //unsigned long newPosition = (360L * valueCount) / STEPS;
    // Calculate new position
    //long newPosition = map(valueCount, 0, 100, 0, STEPS - 1);
  int stepsPerDegree = 3;
  int newPosition = (cumulativeMotorStarts*10*stepsPerDegree) / STEPS;

  motor.setPosition(newPosition);
  while(motor.currentStep != motor.targetStep){
    motor.update(); 
    Serial.println("Callback:");
    Serial.println(newPosition);// This call updates the motor position
    delay(10);
  } 
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");
    pClient->setMTU(517);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("FXXK");
  Serial.println("motor zero set");

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  motor.zero();
  motor.setPosition(STEPS/2);
  Serial.println("Motor starts");
  motor.update();
  delay(1000);

  Wire.begin();
   // Initialize motor to zero position

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  motor.update();
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  motor.setPosition(300);
  while(motor.currentStep != motor.targetStep){
    motor.update(); 
    //Serial.println("Callback:");
    //Serial.println(newPosition);
    delay(10);
  }


  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if(doScan) {
    BLEDevice::getScan()->start(0);
  }

  delay(1000); // Delay a second between loops.
}
