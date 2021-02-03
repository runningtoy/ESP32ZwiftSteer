/*
    Zwift Steering 
*/

//Libraries

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Ticker.h>


/* adapt this values to your setup
- enable DEBUG (#define DEBUG)
- steer full left/full right -> note ADC value and enter hier 
*/
#define MAX_ADC_LEFT 1850
#define MAX_ADC_RIGHT 2900

Ticker watchDOG;
#define watchdogMAXCounter 60*15 // 60minuten
static uint32_t watchdogCounter = watchdogMAXCounter;
#define POWERLATCH 23
#define LED 19

//DEBUG
#define DEBUG

//BLE Definitions
#define STEERING_DEVICE_UUID "347b0001-7635-408b-8918-8ff3949ce592"
#define STEERING_ANGLE_CHAR_UUID "347b0030-7635-408b-8918-8ff3949ce592" //notify
#define STEERING_RX_CHAR_UUID "347b0031-7635-408b-8918-8ff3949ce592"    //write
#define STEERING_TX_CHAR_UUID "347b0032-7635-408b-8918-8ff3949ce592"    //indicate

#define POT 32 // Joystick Xaxis to GPIO32

//Angle calculation parametres
#define MAX_ADC_RESOLUTION 4095 //ESP32 ADC is 12bit
#define MAX_STEER_ANGLE 35
#define ZERO_FLOOR 4

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool auth = false;

float angle = 20;
float old_angle = 20;
//Sterzo stuff
int FF = 0xFF;
uint8_t authChallenge[4] = {0x03, 0x10, 0xff, 0xff};
uint8_t authSuccess[3] = {0x03, 0x11, 0xff};

BLEServer *pServer = NULL;
BLECharacteristic *pAngle = NULL;
BLECharacteristic *pRx = NULL;
BLECharacteristic *pTx = NULL;
BLEAdvertising *pAdvertising;

//Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        BLEDevice::startAdvertising();
        
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

//Characteristic Callbacks
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{

    void onRead(BLECharacteristic *pRx)
    {

    }

    void onWrite(BLECharacteristic *pRx){

        std::string rxValue = pRx->getValue();
        
        if(rxValue.length() == 4){
          delay(250);
          pTx->setValue(authSuccess,3);
          pTx->indicate();
          auth = true;
          #ifdef DEBUG
          Serial.println("Auth Success!");
          #endif
        }
    }
};

//Joystick read angle from axis
float readAngle()
{
    int potVal = analogRead(POT);
    
    // angle = map(potVal,0,MAX_ADC_RESOLUTION,-35,35); //Mapping function
    angle = map(potVal,MAX_ADC_LEFT,2900,MAX_STEER_ANGLE,-1*MAX_STEER_ANGLE); //Mapping function
    
    
    // kwakeham style:
    
    // angle = (((potVal) / (float)MAX_ADC_RESOLUTION) * (MAX_STEER_ANGLE * 2)) - MAX_STEER_ANGLE;
    
    if (fabsf(angle) < ZERO_FLOOR){
        angle = 0;
    }
   
    #ifdef DEBUG
    Serial.print("ADC:");
    Serial.print(potVal);
    Serial.print("\tangle:");
    Serial.println(angle);
    #endif

    return angle;
}


void setupPWR(){
  pinMode(POWERLATCH, OUTPUT);
  // pinMode(LED, OUTPUT);
  // Keeps the circuit on
  digitalWrite(POWERLATCH, HIGH);
  // digitalWrite(LED, HIGH);

  ledcAttachPin(LED, 0);
  ledcSetup(0, 4000, 8); 
  ledcWrite(0, 10);
}

void fct_powerdown(){
  digitalWrite(POWERLATCH, LOW);
  digitalWrite(LED, LOW);
}

void fct_Watchdog() {
  watchdogCounter--;
  Serial.print("Watchdog Counter:");
  Serial.println(watchdogCounter);
  if (watchdogCounter < 1) {
    fct_powerdown();
  }
}

//Arduino setup
void setup()
{
    setupPWR();
    watchDOG.attach(1, fct_Watchdog);
    //Serial Debug
    Serial.begin(115200);
    
    pinMode(POT, INPUT);    // GPIO32 will be => Xaxis on Joystick
    
    
    //Setup BLE
    #ifdef DEBUG
    Serial.println("Creating BLE server...");
    #endif
    BLEDevice::init("STEERING");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    #ifdef DEBUG
    Serial.println("Define service...");
    #endif
    BLEService *pService = pServer->createService(STEERING_DEVICE_UUID);

    // Create BLE Characteristics
    #ifdef DEBUG
    Serial.println("Define characteristics");
    #endif
    pTx = pService->createCharacteristic(STEERING_TX_CHAR_UUID, BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_READ);
    pTx->addDescriptor(new BLE2902());
    
    pRx = pService->createCharacteristic(STEERING_RX_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    pRx->addDescriptor(new BLE2902());
    pRx->setCallbacks(new MyCharacteristicCallbacks());

    pAngle = pService->createCharacteristic(STEERING_ANGLE_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    pAngle->addDescriptor(new BLE2902());

    // Start the service
    #ifdef DEBUG
    Serial.println("Staring BLE service...");
    #endif
    pService->start();

    // Start advertising
    #ifdef DEBUG
    Serial.println("Define the advertiser...");
    #endif
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setScanResponse(true);
    pAdvertising->addServiceUUID(STEERING_DEVICE_UUID);
    pAdvertising->setMinPreferred(0x06); // set value to 0x00 to not advertise this parameter
    #ifdef DEBUG
    Serial.println("Starting advertiser...");
    #endif
    BLEDevice::startAdvertising();
    #ifdef DEBUG
    Serial.println("Waiting a client connection to notify...");
    #endif
}

//Arduino loop
void loop()
{

    if (deviceConnected)
    {
        
        if(auth){
          angle = readAngle();
          if(abs(angle-old_angle)>2){
                watchdogCounter=watchdogMAXCounter;
          }
          Serial.print("angle");
          Serial.println(angle);
          old_angle=angle;
          pAngle->setValue(angle);
          pAngle->notify();
          #ifdef DEBUG
          Serial.print("TX Angle: ");
          Serial.println(angle);
          #endif
          delay(250);
        } else {
          #ifdef DEBUG
          Serial.println("Auth Challenging");
          #endif
          pTx->setValue(authChallenge, 4);
          pTx->indicate();
          delay(250);
        
        }
    }

    //Advertising
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(300);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        #ifdef DEBUG
        Serial.println("Nothing connected, start advertising");
        #endif
        oldDeviceConnected = deviceConnected;
    }
   
    //Connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        oldDeviceConnected = deviceConnected;
        #ifdef DEBUG
        Serial.println("Connecting...");
        #endif
    }

    if (!deviceConnected)
    {
        #ifdef DEBUG
        readAngle();
        #endif
    }
}