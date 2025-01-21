#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 1

RTC_DATA_ATTR int bootCount = 0;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  if (bootCount == 0)
  {
    Serial.println("U");
  }
  else
  {
    Serial.println("U");
  }

  bootCount++;
}

void loop()
{
  Serial.println("D");

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  esp_light_sleep_start();

  Serial.println("U");
  delay(1000);
}
