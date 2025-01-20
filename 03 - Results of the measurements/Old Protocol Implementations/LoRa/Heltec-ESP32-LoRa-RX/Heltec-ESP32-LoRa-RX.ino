
/**
 * Receiver Code: Receives LoRa packets and displays the string along with RSSI
 * and SNR.
 */

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

// Frequency in MHz. Check your own rules and regulations to see what is legal
// where you are.
#define FREQUENCY 866.3 // for Europe
// #define FREQUENCY           905.2       // for US

// LoRa bandwidth. Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5,
// 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH 250.0

// Number from 5 to 12. Higher means slower but higher "processor gain".
#define SPREADING_FACTOR 9

// Transmit power in dBm. 0 dBm = 1 mW.
#define TRANSMIT_POWER 0

String rxdata;
volatile bool rxFlag = false;

void setup() {
  heltec_setup();
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  heltec_loop();

  // If a packet was received, display it and the RSSI and SNR
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());
      display.clear();
      display.drawString(0, 0, "RX: " + rxdata);
      display.drawString(0, 10, "RSSI: " + String(radio.getRSSI()) + " dBm");
      display.drawString(0, 20, "SNR: " + String(radio.getSNR()) + " dB");
      display.display();
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// Can't do Serial or display things here, takes too much time for the interrupt
void rx() { rxFlag = true; }
