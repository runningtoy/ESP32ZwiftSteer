/*

//Zwift steering demo code
//Takes ADC reading from pin 32 and converts to an angle between -40 and +40 and transmits to Zwift via BLE

//Based on BLE Arduino for ESP32 examples (Kolban et al.)
//Keith Wakeham's explanation https://www.youtube.com/watch?v=BPVFjz5zD4g
//Andy's demo code: https://github.com/fiveohhh/zwift-steerer/

//Code written in VSCode with PlatformIO for a Lolin32 Lite
//Should work in Arduino IDE if the #include <Arduino.h> is removed

//Tested using Zwift on Android (Galaxy A5 2017)

 * Copyright 2020 Peter Everett
 * v1.0 Sep 2020 - Initial version
 * 
 * This work is licensed the GNU General Public License v3 (GPL-3)

*/


#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "M5Atom.h"

#define STEERING_DEVICE_UUID "347b0001-7635-408b-8918-8ff3949ce592"
#define STEERING_ANGLE_CHAR_UUID "347b0030-7635-408b-8918-8ff3949ce592"     //notify
/*
//These charateristics are present on the Sterzo but aren't necessary for communication with Zwift
#define STEERING_POWER_CHAR_UUID "347b0012-7635-408b-8918-8ff3949ce592"     //write
#define STEERING_UNKNOWN2_CHAR_UUID "347b0013-7635-408b-8918-8ff3949ce592"  //value 0xFF, read
#define STEERING_UNKNOWN3_CHAR_UUID "347b0014-7635-408b-8918-8ff3949ce592"  //value 0xFF, notify
#define STEERING_UNKNOWN4_CHAR_UUID "347b0019-7635-408b-8918-8ff3949ce592"  //value x0FF, read
*/
#define STEERING_RX_CHAR_UUID "347b0031-7635-408b-8918-8ff3949ce592"  //write
#define STEERING_TX_CHAR_UUID "347b0032-7635-408b-8918-8ff3949ce592"  //indicate


bool deviceConnected = false;
bool oldDeviceConnected = false;
bool auth = false;

double pitch, roll;
double r_rand = 180 / PI;

int FF = 0xFF;
uint8_t authChallenge[4] = {0x03, 0x10, 0xff, 0xff};
uint8_t authSuccess[3] = {0x03, 0x11, 0xff};

BLEServer* pServer = NULL;
BLECharacteristic* pAngle = NULL;
/*
//These charateristics are present on the Sterzo but aren't necessary for communication with Zwift
BLECharacteristic* pPwr = NULL;
BLECharacteristic* pU2 = NULL;
BLECharacteristic* pU3 = NULL;
BLECharacteristic* pU4 = NULL;
*/
BLECharacteristic* pRx = NULL;
BLECharacteristic* pTx = NULL;


//BLEAdvertisementData advert;
//BLEAdvertisementData scan_response;
BLEAdvertising *pAdvertising;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void readAngle() {
    M5.IMU.getAttitude(&pitch, &roll);
    Serial.printf("%.2f,%.2f\n", pitch, roll);
}

void setup() {
 
  //setup pins for Pot
  M5.begin(true, true, true);
  M5.IMU.Init();

  Serial.begin(115200);
  //Setup BLE
  Serial.println("Creating BLE server...");
  BLEDevice::init("STEERING");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  Serial.println("Define service...");
  BLEService *pService = pServer->createService(STEERING_DEVICE_UUID);

  // Create BLE Characteristics
  Serial.println("Define characteristics");
  //The Sterzo includes all of these characteristics, but you only need the Tx and Rx (for the handshaking) and the steerer angle sensor
  /*
  pPwr = pService->createCharacteristic(STEERING_POWER_CHAR_UUID,BLECharacteristic::PROPERTY_WRITE);
  pPwr->addDescriptor(new BLE2902());
  pU2 = pService->createCharacteristic(STEERING_UNKNOWN2_CHAR_UUID,BLECharacteristic::PROPERTY_READ);
  pU2->addDescriptor(new BLE2902());
  pU3 = pService->createCharacteristic(STEERING_UNKNOWN3_CHAR_UUID,BLECharacteristic::PROPERTY_NOTIFY);
  pU3->addDescriptor(new BLE2902());
  pU4 = pService->createCharacteristic(STEERING_UNKNOWN4_CHAR_UUID,BLECharacteristic::PROPERTY_READ);
  pU4->addDescriptor(new BLE2902());*/

  pTx = pService->createCharacteristic(STEERING_TX_CHAR_UUID,BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_READ);
  pTx->addDescriptor(new BLE2902());
  pRx = pService->createCharacteristic(STEERING_RX_CHAR_UUID,BLECharacteristic::PROPERTY_WRITE);
  pRx->addDescriptor(new BLE2902());
  pAngle = pService->createCharacteristic(STEERING_ANGLE_CHAR_UUID,BLECharacteristic::PROPERTY_NOTIFY);
  pAngle->addDescriptor(new BLE2902());

  // Start the service
  Serial.println("Staring BLE service...");
  pService->start();

  // Start advertising
  // Zwift only shows the steering button when the service is advertised
  Serial.println("Define the advertiser...");
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(true);  
  pAdvertising->addServiceUUID(STEERING_DEVICE_UUID);
  pAdvertising->setMinPreferred(0x06);  // set value to 0x00 to not advertise this parameter
  Serial.println("Starting advertiser...");
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
  
}

void loop() {
  readAngle();
  delay(100);
}

void loop_BLE() {
  if (deviceConnected) {
    if (auth) {
      //Connected to Zwift so read the potentiometer and start transmitting the angle
      Serial.print("Transmitting angle: ");
      Serial.println(roll);
      pAngle->setValue(roll);
      pAngle->notify();
      delay(500);
    } else {
      //Not connected to Zwift so start the connectin process
      pTx->setValue(FF);
      pTx->indicate();
      //Do the handshaking
      std::string rxValue = pRx->getValue();
      if (rxValue.length() == 0) {
        Serial.println("No data received");
        delay(250);
      } else {
        Serial.print("Handshaking....");
        if (rxValue[0] == 0x03 && rxValue[1] == 0x10) {
          delay(250);
          //send 0x0310FFFF (the last two octets can be anything)
          pTx->setValue(authChallenge,4);
          pTx->indicate();
          //Zwift will now send 4 bytes as a response, which start with 0x3111
          //We don't really care what it is as long as we get a response
          delay(250);
          rxValue = pRx->getValue();
          if (rxValue.length() == 4) {
            //connected, so send 0x0311ff
            delay(250);
            pTx->setValue(authSuccess,3);
            pTx->indicate();
            auth = true;
            Serial.println("Success!");
          }
        }
      }
    }
      delay(50);  //small delay so BLW stack doesn't get overloaded
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(300); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Nothing connected, start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Connecting...");
    }
    if (!deviceConnected) {
    }
}