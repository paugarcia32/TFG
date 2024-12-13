#include <EEPROM.h>

void setup() {
  Serial.begin(115200); // Inicializamos el puerto serial
  delay(1000);          // Damos tiempo para que el puerto serial se estabilice
  Serial.println("U");  // Mensaje al despertarse
}

void loop() {
  // Llamamos a la función para dormir
  hibernate();
}

void hibernate() {
  int ledIndex = 42; // Ejemplo de valor para guardar
  // Guardamos el valor de ledIndex en la EEPROM
  EEPROM.write(0, ledIndex);
  EEPROM.commit();

  // Enviamos mensaje antes de entrar en modo de sueño
  Serial.println("D");

  // Configuramos los dominios de energía para el sueño profundo

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);

  // Configuramos el temporizador para 10 segundos
  esp_sleep_enable_timer_wakeup(10 * 1000000); // 10 segundos en microsegundos

  // Entramos en sueño profundo
  esp_deep_sleep_start();
}
