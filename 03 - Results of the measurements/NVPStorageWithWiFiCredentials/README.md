# Save WiFi credentials in NVP memory

This project allows an ESP32 to receive Wi-Fi credentials (SSID and Password) over a Bluetooth Low Energy (BLE) connection from another device (e.g., a smartphone) and store them in Non-Volatile Storage (NVS). The ESP32 then attempts to connect to the Wi-Fi network using the received credentials. On restart, it retrieves the stored credentials and reconnects automatically.

## Features

- BLE Communication: Establishes a secure BLE connection to receive data.
- Encrypted Data Transfer: Uses BLE encryption to securely receive Wi-Fi credentials.
- NVS Storage: Stores Wi-Fi credentials in NVS for persistence across resets.
- Auto-Reconnect: Automatically reconnects to the Wi-Fi network on restart if credentials are stored.
- Serial Output: Provides detailed status messages over Serial for debugging.

## Example

![{F8F79039-F047-408D-88A9-0C3ECEDB96F1}](https://github.com/user-attachments/assets/c3c5e8ed-2c7d-4285-b402-82c8d1a58acc)
