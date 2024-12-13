#define uS_TO_S_FACTOR 1000000  /* Conversión de microsegundos a segundos */
#define TIME_TO_SLEEP  1       /* Tiempo que el ESP32 estará dormido (en segundos) */

RTC_DATA_ATTR int bootCount = 0;  // Contador persistente entre reinicios

void setup() {
  Serial.begin(115200);
  // delay(1000);  // Esperar a que el puerto serie esté listo

  if (bootCount == 0) {
    Serial.println("U");  // Primer arranque, consideramos que se ha despertado
  } else {
    Serial.println("U");  // Despertó del deep sleep
  }

  bootCount++;  // Incrementar el contador de arranques

  // Configurar el temporizador para el modo de sueño profundo
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  Serial.println("D");  // Enviamos 'D' antes de entrar en deep sleep
  // delay(1000);          // Pequeño retraso para asegurar que el mensaje se envía
  Serial.flush();       // Vaciar el buffer serie para enviar datos pendientes

  esp_deep_sleep_start();  // Entrar en modo de sueño profundo
}

void loop() {
  // Este código no se ejecutará debido al deep sleep
}
