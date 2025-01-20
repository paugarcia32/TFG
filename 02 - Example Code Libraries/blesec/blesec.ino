

#include <ArduinoBLE.h>

// BLE Service
BLEService testService("180F");

// BLE String Characteristic
BLEStringCharacteristic stringCharacteristic("183E", BLERead | BLEWrite, 31);

// BLE Secure Characteristic
BLEUnsignedCharCharacteristic secureValue("2a3F",
                                          BLERead | BLEWrite | BLEEncryption);

// BLE Characteristic to receive data from mobile
BLEStringCharacteristic dataCharacteristic("2A37", BLERead | BLEWrite, 31);

void generateRandomKey(uint8_t *key, size_t length) {
  for (size_t i = 0; i < length; i++) {
    key[i] = random(0, 256);
  }
}

void onDataReceived(BLEDevice central, BLECharacteristic characteristic) {
  int length = characteristic.valueLength();
  uint8_t value[length];
  characteristic.readValue(value, length);

  char buffer[length + 1]; // +1 para el terminador nulo
  memcpy(buffer, value, length);
  buffer[length] = '\0'; // agregar el terminador nulo

  String receivedData = String(buffer);
  Serial.print("Received data: ");
  Serial.println(receivedData);
}

void setup() {
  Serial.begin(115200); // initialize serial communication
  while (!Serial)
    ;

  Serial.println("Serial connected");

  // Callback function with confirmation code when new device is pairing.
  BLE.setDisplayCode([](uint32_t confirmCode) {
    Serial.println("New device pairing request.");
    Serial.print("Confirm code matches pairing device: ");
    char code[6];
    sprintf(code, "%06d", confirmCode);
    Serial.println(code);
  });

  // Callback to allow accepting or rejecting pairing
  BLE.setBinaryConfirmPairing([]() {
    Serial.print("Should we confirm pairing? ");
    delay(5000);
    Serial.println("yes");
    return true;
  });

  BLE.setGetIRKs([](uint8_t *nIRKs, uint8_t **BDaddrTypes, uint8_t ***BDAddrs,
                    uint8_t ***IRKs) {
    *nIRKs = 2;

    *BDAddrs = new uint8_t *[*nIRKs];
    *IRKs = new uint8_t *[*nIRKs];
    *BDaddrTypes = new uint8_t[*nIRKs];

    uint8_t device1Mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device1IRK[16];
    generateRandomKey(device1IRK, 16);

    uint8_t device2Mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2IRK[16];
    generateRandomKey(device2IRK, 16);

    (*BDaddrTypes)[0] = 0;
    (*BDAddrs)[0] = new uint8_t[6];
    (*IRKs)[0] = new uint8_t[16];
    memcpy((*IRKs)[0], device1IRK, 16);
    memcpy((*BDAddrs)[0], device1Mac, 6);

    (*BDaddrTypes)[1] = 0;
    (*BDAddrs)[1] = new uint8_t[6];
    (*IRKs)[1] = new uint8_t[16];
    memcpy((*IRKs)[1], device2IRK, 16);
    memcpy((*BDAddrs)[1], device2Mac, 6);

    return 1;
  });

  BLE.setGetLTK([](uint8_t *address, uint8_t *LTK) {
    Serial.print("Received request for address: ");
    for (int i = 0; i < 6; i++) {
      Serial.print(address[i], HEX);
      if (i < 5)
        Serial.print(":");
    }
    Serial.println();

    uint8_t device1Mac[6] = {0x7c, 0x7f, 0x21, 0xf9, 0x65, 0xc5};
    uint8_t device1LTK[16];
    generateRandomKey(device1LTK, 16);

    uint8_t device2Mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2LTK[16];
    generateRandomKey(device2LTK, 16);

    if (memcmp(device1Mac, address, 6) == 0) {
      memcpy(LTK, device1LTK, 16);
      return 1;
    } else if (memcmp(device2Mac, address, 6) == 0) {
      memcpy(LTK, device2LTK, 16);
      return 1;
    }
    return 0;
  });

  BLE.setStoreIRK([](uint8_t *address, uint8_t *IRK) {
    Serial.print(F("New device with MAC : "));
    for (int i = 0; i < 6; i++) {
      Serial.print(address[i], HEX);
      if (i < 5)
        Serial.print(":");
    }
    Serial.println();
    Serial.print(F("Need to store IRK   : "));
    for (int i = 0; i < 16; i++) {
      Serial.print(IRK[i], HEX);
      if (i < 15)
        Serial.print(":");
    }
    Serial.println();
    return 1;
  });

  BLE.setStoreLTK([](uint8_t *address, uint8_t *LTK) {
    Serial.print(F("New device with MAC : "));
    for (int i = 0; i < 6; i++) {
      Serial.print(address[i], HEX);
      if (i < 5)
        Serial.print(":");
    }
    Serial.println();
    Serial.print(F("Need to store LTK   : "));
    for (int i = 0; i < 16; i++) {
      Serial.print(LTK[i], HEX);
      if (i < 15)
        Serial.print(":");
    }
    Serial.println();
    return 1;
  });

  while (1) {
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
    testService.addCharacteristic(dataCharacteristic);

    BLE.addService(testService);

    stringCharacteristic.writeValue("string");
    secureValue.writeValue(0);

    dataCharacteristic.setEventHandler(BLEWritten, onDataReceived);

    delay(1000);

    BLE.setPairable(false);

    if (!BLE.advertise()) {
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
    Serial.println("Connected to central");

    while (central.connected()) {
      // wait for central to disconnect
    }

    Serial.println("Disconnected from central");
  }
}
