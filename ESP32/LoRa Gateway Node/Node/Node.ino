#include <RadioLib.h>

SX1262 radio = new Module(8, 14, 12, 13);
volatile bool receivedFlag = false;
volatile bool transmitting = false;
int currentSF = 12;
const uint16_t nodeID = random(1, 65535);

// Número máximo de reintentos
const int maxRetries = 5;

void setFlag(void) {
  if (!transmitting) {
    receivedFlag = true;
  }
}

void setup() {
  Serial.begin(115200);

  Serial.print(F("[SX1262] Inicializando ... "));
  int state = radio.begin(868.0);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡éxito!"));
  } else {
    Serial.print(F("fallo, código "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // Configurar parámetros del radio
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setCRC(true);
  radio.setSyncWord(0x12);

  radio.setDio1Action(setFlag);

  Serial.print(F("[SX1262] Iniciando recepción ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡éxito!"));
  } else {
    Serial.print(F("fallo, código "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void loop() {
  if (receivedFlag) {
    receivedFlag = false;

    String receivedData;
    int state = radio.readData(receivedData);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[SX1262] Paquete recibido: "));
      Serial.println(receivedData);

      if (receivedData == "BEACON") {
        int randomDelay = random(0, 2000);
        Serial.print(F("[SX1262] Esperando "));
        Serial.print(randomDelay);
        Serial.println(F(" ms antes de enviar ACK."));
        delay(randomDelay);

        enviarAck();
      } else if (receivedData.startsWith("SCHEDULE:")) {
        int idStart = receivedData.indexOf(':') + 1;
        int idEnd = receivedData.indexOf(':', idStart);
        String idStr = receivedData.substring(idStart, idEnd);
        uint16_t receivedID = idStr.toInt();

        if (receivedID == nodeID) {
          int delayStart = idEnd + 1;
          int delayEnd = receivedData.indexOf(':', delayStart);
          String delayStr = receivedData.substring(delayStart, delayEnd);
          unsigned long delayTime = delayStr.toInt();

          String sfStr = receivedData.substring(delayEnd + 1);
          uint8_t newSF = sfStr.toInt();

          Serial.print(F("[SX1262] Programando envío de datos en "));
          Serial.print(delayTime);
          Serial.println(F(" ms."));
          Serial.print(F("[SX1262] Actualizando SF a: SF"));
          Serial.println(newSF);

          // Actualizar SF
          currentSF = newSF;
          radio.setSpreadingFactor(currentSF);

          // Programar el envío de datos
          delay(delayTime);
          enviarDatos();
        }
      } else {
        Serial.println(F("[SX1262] Datos inesperados recibidos."));
      }

      // Volver al modo recepción
      radio.startReceive();
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println(F("[SX1262] Error de CRC!"));
      radio.startReceive();
    } else {
      Serial.print(F("Fallo, código "));
      Serial.println(state);
      radio.startReceive();
    }
  }
}

void enviarAck() {
  radio.standby();
  transmitting = true;

  Serial.println(F("[SX1262] Enviando ACK..."));

  String ackMessage = "ACK:" + String(nodeID);
  int state = radio.transmit(ackMessage);

  transmitting = false;

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] ACK enviado con éxito!"));
  } else {
    Serial.print(F("Fallo al enviar ACK, código "));
    Serial.println(state);
  }

  radio.startReceive();
}

void enviarDatos() {
  radio.standby();
  transmitting = true;

  Serial.println(F("[SX1262] Enviando datos..."));

  // Preparar tus datos aquí
  String dataMessage = "DATA:" + String(nodeID) + ":<tus_datos>";

  int state = radio.transmit(dataMessage);

  transmitting = false;

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Datos enviados con éxito!"));
  } else {
    Serial.print(F("Fallo al enviar datos, código "));
    Serial.println(state);
  }

  // Restablecer el SF a 12 después de enviar los datos
  currentSF = 12;
  radio.setSpreadingFactor(currentSF);
  Serial.println(F("[SX1262] Spreading Factor reiniciado a 12."));

  // Volver al modo recepción
  radio.startReceive();
}
