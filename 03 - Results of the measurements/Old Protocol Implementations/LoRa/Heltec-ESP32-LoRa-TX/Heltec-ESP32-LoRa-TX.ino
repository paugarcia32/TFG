
/**
 * Transmitter Code: Sends LoRa packets with a custom string and displays status
 * on screen.
 */

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

// Pause between transmitted packets in seconds.
#define PAUSE 300

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

long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

// Custom message to send
String customMessage = "Hello, this is a test message";

void setup() {
  heltec_setup();
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
}

void loop() {
  heltec_loop();

  bool tx_legal = millis() > last_tx + minimum_pause;
  // Transmit a packet every PAUSE seconds or when the button is pressed
  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) ||
      button.isSingleClick()) {
    // In case of button click, tell user to wait
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n",
                  (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      display.clear();
      display.drawString(
          0, 0,
          "Legal limit, wait " +
              String((int)((minimum_pause - (millis() - last_tx)) / 1000) + 1) +
              " sec.");
      display.display();
      return;
    }
    String messageToSend = customMessage + " [" + String(counter) + "]";
    both.printf("TX [%s] ", messageToSend.c_str());
    display.clear();
    display.drawString(0, 0, "TX [" + String(counter) + "]");
    radio.clearDio1Action();
    heltec_led(50); // 50% brightness is plenty for this LED
    tx_time = millis();
    RADIOLIB(radio.transmit(messageToSend.c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
      display.drawString(0, 10, "OK (" + String((int)tx_time) + " ms)");
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
      display.drawString(0, 10, "fail (" + String(_radiolib_status) + ")");
    }
    display.display();
    // Maximum 1% duty cycle
    minimum_pause = tx_time * 100;
    last_tx = millis();
    counter++;
  }
}
