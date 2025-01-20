#include <ArduinoBLE.h>
#include <WiFi.h>
#include <Preferences.h> // Include Preferences library for NVS storage

// Custom BLE Service UUID
// BLEService wifiConfigService("e1f5055c-b509-4c7d-b951-3a8eebd40529");
BLEService wifiConfigService("0x008D");

// Custom BLE Characteristic UUID for Wi-Fi Credentials
BLEStringCharacteristic wifiCredentialsCharacteristic(
  "c6e0bf23-f27d-4e1a-9ed8-fdfbd1b81e33",
  BLERead | BLEWrite | BLEEncryption, // Require encryption
  128 // Adjust size to accommodate longer credentials
);

Preferences preferences; // Create a Preferences object for NVS storage

void onDataReceived(BLEDevice central, BLECharacteristic characteristic) {
  // Ensure we're handling the correct characteristic
  if (characteristic.uuid() == wifiCredentialsCharacteristic.uuid()) {
    // Get the value
    int length = characteristic.valueLength();
    const uint8_t* data = characteristic.value();

    // Create a buffer to hold the data plus a null terminator
    char buffer[length + 1]; // +1 for null terminator
    memcpy(buffer, data, length);
    buffer[length] = '\0'; // Null-terminate the buffer

    String receivedData = String(buffer);

    Serial.print("Received data: ");
    Serial.println(receivedData);

    // Expecting data in the format "SSID:Password"
    int separatorIndex = receivedData.indexOf(':');

    if (separatorIndex > 0) {
      String ssid = receivedData.substring(0, separatorIndex);
      String password = receivedData.substring(separatorIndex + 1);

      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(password);

      // Store credentials in NVS
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);

      // Attempt to connect to Wi-Fi
      WiFi.begin(ssid.c_str(), password.c_str());

      Serial.println("Connecting to Wi-Fi...");
      int maxTries = 20;
      while (WiFi.status() != WL_CONNECTED && maxTries > 0) {
        delay(500);
        Serial.print(".");
        maxTries--;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("\nFailed to connect to Wi-Fi.");
      }
    } else {
      Serial.println("Invalid data format. Expected 'SSID:Password'.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Serial connected");

  preferences.begin("wifiCreds", false); // Initialize NVS storage

  // Retrieve stored Wi-Fi credentials
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");

  if (ssid != "") {
    Serial.println("Found stored Wi-Fi credentials.");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.println("Attempting to connect to Wi-Fi...");
    int maxTries = 20;
    while (WiFi.status() != WL_CONNECTED && maxTries > 0) {
      delay(500);
      Serial.print(".");
      maxTries--;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect to Wi-Fi.");
    }
  } else {
    Serial.println("No stored Wi-Fi credentials.");
  }

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  Serial.println("BLE initialized");

  // Set BLE device name and local name
  BLE.setDeviceName("ESP32 Wi-Fi Config");
  BLE.setLocalName("ESP32 Wi-Fi Config");

  // Set the advertised service
  BLE.setAdvertisedService(wifiConfigService);

  // Add the characteristic to the service
  wifiConfigService.addCharacteristic(wifiCredentialsCharacteristic);

  // Add the service
  BLE.addService(wifiConfigService);

  // Set the event handler for the characteristic
  wifiCredentialsCharacteristic.setEventHandler(BLEWritten, onDataReceived);

  // Start advertising
  BLE.advertise();

  Serial.println("Advertising BLE service...");
}

void loop() {
  // Listen for BLE peripherals to connect
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    // While the central is connected
    while (central.connected()) {
      // Process BLE events
      BLE.poll();
      delay(100);
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
