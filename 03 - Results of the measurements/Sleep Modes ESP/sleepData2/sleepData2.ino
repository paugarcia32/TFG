#define uS_TO_S_FACTOR 1000000  /* Conversión de microsegundos a segundos */
#define TIME_TO_SLEEP  1       /* Tiempo que el ESP32 estará dormido (en segundos) */

RTC_DATA_ATTR int bootCount = 0;  // Contador persistente entre reinicios

void setup() {
  Serial.begin(115200);
  // delay(1000);  // Dar tiempo al Serial para inicializarse correctamente

  if (bootCount == 0) {
    Serial.println("U");  // Primer arranque
  } else {
    Serial.println("U");  // Se despertó del modo de sueño ligero
  }

  bootCount++;  // Incrementar el contador de arranques
}

void loop() {
  // Imprimir "D" antes de entrar en sueño ligero
  Serial.println("D");
  // delay(1000);  // Asegurarnos de que el mensaje se envía

  // Configurar el temporizador para despertar después de un tiempo
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Entrar en modo light sleep
  esp_light_sleep_start();

  // Una vez que el ESP32 despierte, continuará desde aquí
  Serial.println("U");
  delay(1000);  // Esperar antes de volver a dormir
}
