// Turns the 'PRG' button into the power button, long press is off 
#define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"

// Uncomment this if you have Wireless Stick v3
// #define HELTEC_WIRELESS_STICK

// creates 'radio', 'display' and 'button' instances 
#include <heltec_unofficial.h>

void setup() {
      Serial.begin(115200);
  heltec_setup();
  Serial.println("Serial works");
  // Display
  display.println("Display works");
  // Radio
  display.print("Radio ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    display.println("works");
  } else {
    display.printf("fail, code: %i\n", state);
  }
  // Battery
  float vbat = heltec_vbat();
  display.printf("Vbat: %.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));
}

void loop() {
  heltec_loop();
  // Button
  if (button.isSingleClick()) {
    display.println("Button works");
    // LED
    for (int n = 0; n <= 100; n++) { heltec_led(n); delay(5); }
    for (int n = 100; n >= 0; n--) { heltec_led(n); delay(5); }
    display.println("LED works");
  }
}
