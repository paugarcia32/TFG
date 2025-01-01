/*
   Nodo B (Receptor con CAD) - SX126x + RadioLib
   Versión "limpia": sólo imprime información relevante 
   al recibir un paquete o si ocurre un error.
*/

#include <RadioLib.h>


#define NSS_PIN   8
#define DIO1_PIN  14
#define NRST_PIN  12
#define BUSY_PIN  13

SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);

// Flag para saber si estamos recibiendo
bool receiving = false;
// Flag para indicar que algo ocurrió (detección CAD o timeout)
volatile bool cadFlag = false;

// Función de interrupción
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  cadFlag = true;
}

void setup() {
  Serial.begin(115200);
  while(!Serial);


  int state = radio.begin(868.0, 125.0, 7, 5);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[RX] Error al inicializar la radio. Código: ");
    Serial.println(state);
    while(true);
  }

  // Activar la interrupción en DIO1 para Channel Activity Detection (CAD)
  radio.setDio1Action(setFlag);

  // Iniciar escaneo (CAD) de forma continua
  radio.startChannelScan();
}

void loop() {
  if(cadFlag) {
    // Reiniciamos el flag
    cadFlag = false;

    // Leemos el resultado de CAD
    int cadResult = radio.getChannelScanResult();

    if(receiving) {
      // Si ya estábamos en modo recepción, significa que llegó un paquete
      receiving = false;

      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        Serial.print("[RX] Paquete recibido -> ");
        Serial.println(str);
      } else {
        Serial.print("[RX] Error al leer paquete. Código: ");
        Serial.println(state);
      }

      // Reiniciar CAD
      radio.startChannelScan();

    } else {
      // No estábamos en recepción, así que el CAD finalizó
      if (cadResult == RADIOLIB_LORA_DETECTED) {
        // Iniciamos recepción
        int state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
          receiving = true;
        } else {
          Serial.print("[RX] Error al iniciar recepción. Código: ");
          Serial.println(state);
          radio.startChannelScan();
        }
      } else if (cadResult == RADIOLIB_CHANNEL_FREE) {
        // Canal libre; seguir escaneando
        radio.startChannelScan();
      } else {
        // Error en CAD; intentar reiniciar
        radio.startChannelScan();
      }
    }
  }
}
