#include <ArduinoBLE.h>

#define CTRL_LED LED_BUILTIN

// BLE Service
BLEService testService("180F");

// BLE String Characteristic
BLEStringCharacteristic stringCharacteristic("183E", BLERead | BLEWrite, 31);

// BLE Secure Characteristic
BLEUnsignedCharCharacteristic secureValue("2a3F", BLERead | BLEWrite | BLEEncryption);

void setup() {
  Serial.begin(115200);    // initialize serial communication
  while (!Serial);

  pinMode(CTRL_LED, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected

  Serial.println("Serial connected");
  
  // Callback function with confirmation code when new device is pairing.
  BLE.setDisplayCode([](uint32_t confirmCode){
    Serial.println("New device pairing request.");
    Serial.print("Confirm code matches pairing device: ");
    char code[6];
    sprintf(code, "%06d", confirmCode);
    Serial.println(code);
  });
  
  // Callback to allow accepting or rejecting pairing
  BLE.setBinaryConfirmPairing([](){
    Serial.print("Should we confirm pairing? ");
    delay(5000);
    Serial.println("yes");
    return true;
  });

  // IRKs are keys that identify the true owner of a random mac address.
  // Add IRKs of devices you are bonded with.
  BLE.setGetIRKs([](uint8_t* nIRKs, uint8_t** BDaddrTypes, uint8_t*** BDAddrs, uint8_t*** IRKs){
    // Set to number of devices
    *nIRKs       = 2;

    *BDAddrs     = new uint8_t*[*nIRKs];
    *IRKs        = new uint8_t*[*nIRKs];
    *BDaddrTypes = new uint8_t[*nIRKs];

    // Set these to the mac and IRK for your bonded devices as printed in the serial console after bonding.
    uint8_t device1Mac[6]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device1IRK[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t device2Mac[6]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2IRK[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


    (*BDaddrTypes)[0] = 0; // Type 0 is for pubc address, type 1 is for static random
    (*BDAddrs)[0] = new uint8_t[6]; 
    (*IRKs)[0]    = new uint8_t[16];
    memcpy((*IRKs)[0]   , device1IRK,16);
    memcpy((*BDAddrs)[0], device1Mac, 6);


    (*BDaddrTypes)[1] = 0;
    (*BDAddrs)[1] = new uint8_t[6];
    (*IRKs)[1]    = new uint8_t[16];
    memcpy((*IRKs)[1]   , device2IRK,16);
    memcpy((*BDAddrs)[1], device2Mac, 6);


    return 1;
  });
  // The LTK is the secret key which is used to encrypt bluetooth traffic
  BLE.setGetLTK([](uint8_t* address, uint8_t* LTK){
    // address is input
    Serial.print("Received request for address: ");
    btct.printBytes(address,6);

    // Set these to the MAC and LTK of your devices after bonding.
    // uint8_t device1Mac[6]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // uint8_t device1LTK[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device1Mac[6]  = {0x7c, 0x7f, 0x21, 0xf9, 0x65, 0xc5};
    uint8_t device1LTK[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2Mac[6]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2LTK[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    

    if(memcmp(device1Mac, address, 6) == 0) {
      memcpy(LTK, device1LTK, 16);
      return 1;
    }else if(memcmp(device2Mac, address, 6) == 0) {
      memcpy(LTK, device2LTK, 16);
      return 1;
    }
    return 0;
  });
  BLE.setStoreIRK([](uint8_t* address, uint8_t* IRK){
    Serial.print(F("New device with MAC : "));
    btct.printBytes(address,6);
    Serial.print(F("Need to store IRK   : "));
    btct.printBytes(IRK,16);
    return 1;
  });
  BLE.setStoreLTK([](uint8_t* address, uint8_t* LTK){
    Serial.print(F("New device with MAC : "));
    btct.printBytes(address,6);
    Serial.print(F("Need to store LTK   : "));
    btct.printBytes(LTK,16);
    return 1;
  });

  while(1){
    if (!BLE.begin()) {
      Serial.println("starting BLE failed!");
      delay(200);
      continue;
    }
    Serial.println("BT init");
    delay(200);

    BLE.setDeviceName("ESP32");
    BLE.setLocalName("ESP32");

    BLE.setAdvertisedService(testService);
    testService.addCharacteristic(stringCharacteristic);
    testService.addCharacteristic(secureValue);

    BLE.addService(testService);

    stringCharacteristic.writeValue("string");
    secureValue.writeValue(0);

    delay(1000);

    BLE.setPairable(false);

    if(!BLE.advertise()){
      Serial.println("failed to advertise bluetooth.");
      BLE.stopAdvertise();
      delay(500);
    } else {
      Serial.println("advertising...");
      break;
    }
    BLE.end();
    delay(100);
  }
}

void loop() {
  BLEDevice central = BLE.central();

  if (central && central.connected()) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    long currentMillis = millis();
    digitalWrite(CTRL_LED, secureValue.value() > 0 ? HIGH : LOW);
  } else {
    digitalWrite(CTRL_LED, LOW);
  }
}
