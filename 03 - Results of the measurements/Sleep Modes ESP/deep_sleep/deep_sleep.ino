#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 1

RTC_DATA_ATTR int bootCount = 0;

void setup()
{
  Serial.begin(115200);

  if (bootCount == 0)
  {
    Serial.println("U");
  }
  else
  {
    Serial.println("U");
  }

  bootCount++;

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.println("D");
  Serial.flush();

  esp_deep_sleep_start();
}

void loop()
{
}
