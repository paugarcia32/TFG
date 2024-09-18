#include <EEPROM.h>

float sensorValue;
uint16_t eeAddress;  // EEPROM address pointer (using uint16_t for consistent 2-byte size)

const uint16_t eeAddressLocation = 0;  // EEPROM address where eeAddress is stored
const uint16_t dataStartAddress = 2;   // Data starts after storing eeAddress (2 bytes)
const uint16_t dataSize = sizeof(float) + sizeof(byte);  // Size of each data entry (float + checksum)
const uint16_t EEPROM_SIZE = 512; // Define the size of EEPROM

// Function to calculate a checksum (simple sum of bytes)
byte calculateChecksum(byte *data, size_t length) {
  byte checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum += data[i];
  }
  return checksum;
}

void setup()
{
  Serial.begin(115200); // Set baud rate to 115200
  while (!Serial); // Wait for Serial to initialize (optional, for boards that need it)
  delay(100); // Small delay to ensure Serial is ready

  // Initialize EEPROM with defined size
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    while (true); // Stay here if EEPROM initialization fails
  }

  // Read the last used eeAddress from EEPROM
  eeAddress = EEPROM.read(eeAddressLocation) | (EEPROM.read(eeAddressLocation + 1) << 8);
  if (eeAddress < dataStartAddress || eeAddress + dataSize > EEPROM_SIZE) {
    eeAddress = dataStartAddress; // Initialize if eeAddress is invalid
  }

  // Optional: Read and print existing data from EEPROM
  Serial.println("Existing data in EEPROM:");
  uint16_t readAddress = dataStartAddress;
  while (readAddress + dataSize <= EEPROM_SIZE) {
    byte floatBytes[sizeof(float)];
    for (size_t i = 0; i < sizeof(float); i++) {
      floatBytes[i] = EEPROM.read(readAddress + i);
    }
    byte storedChecksum = EEPROM.read(readAddress + sizeof(float));

    // Verify checksum
    byte calculatedChecksum = calculateChecksum(floatBytes, sizeof(float));
    if (storedChecksum == calculatedChecksum) {
      float storedValue;
      memcpy(&storedValue, floatBytes, sizeof(float));
      Serial.print("Address ");
      Serial.print(readAddress);
      Serial.print(": ");
      Serial.println(storedValue);
    } else {
      // Invalid data detected; stop reading
      break;
    }

    readAddress += dataSize;
  }

  Serial.println("Enter a value to store in EEPROM:");
}

void loop()
{
  // Check if data is available at the Serial port
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim(); // Remove any leading/trailing whitespace

    // Try to parse the input as a float
    sensorValue = inputString.toFloat();

    // Check if the input is valid
    if (inputString.length() == 0 || (sensorValue == 0.0 && inputString != "0" && inputString != "0.0")) {
      // No valid conversion could be performed
      Serial.println("Invalid input. Please enter a valid number.");
    } else {
      // Valid float value obtained
      Serial.print("Parsed value: ");
      Serial.println(sensorValue);

      // Convert float to byte array
      byte floatBytes[sizeof(float)];
      memcpy(floatBytes, &sensorValue, sizeof(float));

      // Calculate checksum
      byte checksum = calculateChecksum(floatBytes, sizeof(float));

      // Write the value bytes to EEPROM
      for (size_t i = 0; i < sizeof(float); i++) {
        EEPROM.write(eeAddress + i, floatBytes[i]);
      }

      // Write checksum
      EEPROM.write(eeAddress + sizeof(float), checksum);

      // Commit changes to EEPROM
      if (!EEPROM.commit()) {
        Serial.println("Failed to commit changes to EEPROM!");
      } else {
        // Small delay to ensure EEPROM write completes
        delay(5);

        // Read back the value bytes from EEPROM
        byte readBytes[sizeof(float)];
        for (size_t i = 0; i < sizeof(float); i++) {
          readBytes[i] = EEPROM.read(eeAddress + i);
        }

        // Read back the checksum
        byte readChecksum = EEPROM.read(eeAddress + sizeof(float));

        // Verify checksum
        byte calculatedChecksum = calculateChecksum(readBytes, sizeof(float));
        if (readChecksum != calculatedChecksum) {
          Serial.println("Data integrity check failed!");
          // Handle error (e.g., retry write or log the error)
        } else {
          // Reconstruct float value from bytes
          float readValue;
          memcpy(&readValue, readBytes, sizeof(float));
          Serial.print("Stored value at address ");
          Serial.print(eeAddress);
          Serial.print(": ");
          Serial.println(readValue);
        }

        // Update eeAddress for next write
        eeAddress += dataSize;
        if (eeAddress + dataSize > EEPROM_SIZE) {
          eeAddress = dataStartAddress; // Wrap around to the beginning
        }

        // Save the updated eeAddress back to EEPROM
        EEPROM.write(eeAddressLocation, eeAddress & 0xFF);
        EEPROM.write(eeAddressLocation + 1, (eeAddress >> 8) & 0xFF);

        // Commit the updated eeAddress
        if (!EEPROM.commit()) {
          Serial.println("Failed to commit eeAddress to EEPROM!");
        }
      }

      Serial.println("Enter a value to store in EEPROM:");
    }
  }
}
