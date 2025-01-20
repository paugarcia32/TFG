#include <RadioLib.h>

SX1262 radio = new Module(8, 14, 12, 13);

volatile bool receivedFlag = false;
volatile bool transmitting = false;
uint8_t currentSF = 12;
uint16_t nodeID;

void setFlag(void) {
  if (!transmitting) {
    receivedFlag = true;
  }
}

void setup() {
  Serial.begin(115200);

  randomSeed(analogRead(0));
  nodeID = random(1, 65535);

  Serial.print(F("[SX1262] Inicializando ... "));
  int state = radio.begin(868.0);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡éxito!"));
  } else {
    Serial.print(F("fallo, código "));
    Serial.println(state);
    while (true) { delay(10); }
  }

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

    uint8_t receivedData[64];
    int state = radio.readData(receivedData, sizeof(receivedData));

    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[SX1262] Paquete recibido: "));
      Serial.println((char*)receivedData);

      if (strcmp((char*)receivedData, "B") == 0) {
        int randomDelay = random(0, 2000);
        Serial.print(F("[SX1262] Esperando "));
        Serial.print(randomDelay);
        Serial.println(F(" ms antes de enviar ACK."));
        delay(randomDelay);

        enviarAck();
      } else if (strncmp((char*)receivedData, "S:", 2) == 0) {
        procesarSchedule((char*)receivedData);
      } else {
        Serial.println(F("[SX1262] Datos inesperados recibidos."));
      }

      radio.startReceive();
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println(F("[SX1262] Error de CRC!"));
      radio.startReceive();
    } else {
      Serial.print(F("Fallo al recibir datos, código "));
      Serial.println(state);
      radio.startReceive();
    }
  }
}

void enviarAck() {
  radio.standby();
  transmitting = true;

  Serial.println(F("[SX1262] Enviando ACK..."));

  char ackMessage[20];
  sprintf(ackMessage, "A:%u", nodeID);

  int state = radio.transmit((uint8_t*)ackMessage, strlen(ackMessage));

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

  char dataMessage[30];
  sprintf(dataMessage, "D:%u:33ºC", nodeID);

  int state = radio.transmit((uint8_t*)dataMessage, strlen(dataMessage));

  transmitting = false;

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Datos enviados con éxito!"));
  } else {
    Serial.print(F("Fallo al enviar datos, código "));
    Serial.println(state);
  }


  currentSF = 12;
  radio.setSpreadingFactor(currentSF);
  Serial.println(F("[SX1262] Spreading Factor reiniciado a 12."));

  radio.startReceive();
}

void procesarSchedule(char* receivedData) {
  char* token;
  uint16_t receivedID;
  unsigned long delayTime;
  uint8_t newSF;

  token = strtok(receivedData + 2, ":");
  if (token != NULL) {
    receivedID = atoi(token);
  } else {
    Serial.println(F("[SX1262] Formato de mensaje S incorrecto."));
    return;
  }


  if (receivedID == nodeID) {
    token = strtok(NULL, ":");
    if (token != NULL) {
      delayTime = atol(token);
    } else {
      Serial.println(F("[SX1262] Formato de mensaje S incorrecto."));
      return;
    }

    token = strtok(NULL, ":");
    if (token != NULL) {
      newSF = atoi(token);
    } else {
      Serial.println(F("[SX1262] Formato de mensaje S incorrecto."));
      return;
    }

    Serial.print(F("[SX1262] Programando envío de datos en "));
    Serial.print(delayTime);
    Serial.println(F(" ms."));
    Serial.print(F("[SX1262] Actualizando SF a: SF"));
    Serial.println(newSF);

    currentSF = newSF;
    radio.setSpreadingFactor(currentSF);

    unsigned long startTime = millis();
    while (millis() - startTime < delayTime) {

      delay(10);
    }

    enviarDatos();
  } else {
    Serial.println(F("[SX1262] Mensaje S no dirigido a este nodo."));
  }
}
