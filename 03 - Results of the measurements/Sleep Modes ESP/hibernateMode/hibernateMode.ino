#include <EEPROM.h>

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("U");
}

void loop()
{
  hibernate();
}

void hibernate()
{
  int ledIndex = 42;
  EEPROM.write(0, ledIndex);
  EEPROM.commit();

  Serial.println("D");

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

  esp_sleep_enable_timer_wakeup(10 * 1000000);

  esp_deep_sleep_start();
}
