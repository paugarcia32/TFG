
struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button1 = {0, 0, false};

void IRAM_ATTR isr() {
  button1.numberKeyPresses++;
  button1.pressed = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(button1.PIN, INPUT_PULLUP);
  attachInterrupt(button1.PIN, isr, FALLING);
}

void loop() {
  if (button1.pressed) {
    Serial.print("Button has been pressed ");
    Serial.print(button1.numberKeyPresses);
    Serial.println(" times");
    button1.pressed = false;
  }
}
