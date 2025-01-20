#include <Arduino.h>
#include <RadioLib.h>

enum MessageType : uint8_t {
  BEACON = 0b00,
  ACK_MSG = 0b01,
  SCHEDULE = 0b10,
  DATA = 0b11
};

enum DataMessageType : uint8_t {
  DATA_TEMPERATURE = 0b00,
  DATA_PRESSURE = 0b01
};

enum NodeID : uint8_t {
  NODE_0 = 0b00,
  NODE_1 = 0b01,
  NODE_2 = 0b10,
  NODE_3 = 0b11
};

struct Message {
  MessageType type;            // Tipo de mensaje
  NodeID nodeId;               // ID del nodo
  uint8_t sf;                  // Spreading Factor (para SCHEDULE)
  DataMessageType dataType;    // Tipo de mensaje de datos
  bool sign;                   // Signo (para temperatura)
  uint16_t dataValue;          // Valor de temperatura o presión
  uint8_t payload[255];        // Payload adicional
  uint8_t payloadLength;       // Longitud del payload
};

const NodeID nodeId = NODE_0; // Cambia esto según el ID de tu nodo (NODE_0 a NODE_3)

SX1262 radio = new Module(8, 14, 12, 13);

volatile bool operationDone = false;
int operationState = RADIOLIB_ERR_NONE;

enum RadioOperation {
  RADIO_IDLE,
  RADIO_RX,
  RADIO_TX
};

RadioOperation currentOperation = RADIO_IDLE;

#if defined(ESP8266) || defined(ESP32)
  IRAM_ATTR
#endif
void setFlag(void) {
  operationDone = true;
}

void packMessage(const Message& msg, uint8_t* buffer, uint8_t& bufferLength) {
  buffer[0] = 0;

  if (msg.type == BEACON) {
    buffer[0] |= ((uint8_t)msg.type) << 6; // Bits 7 y 6
    bufferLength = 1; // Solo un byte
  } else if (msg.type == ACK_MSG) {
    buffer[0] |= ((uint8_t)msg.type) << 6;   // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4; // Bits 5 y 4
    bufferLength = 1; // Solo un byte
  } else if (msg.type == SCHEDULE) {
    buffer[0] |= ((uint8_t)msg.type) << 6;       // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;     // Bits 5 y 4
    buffer[0] |= ((uint8_t)msg.sf) & 0b00000111; // Bits 2,1,0
    bufferLength = 1; // Solo un byte
  } else if (msg.type == DATA) {
    buffer[0] |= ((uint8_t)msg.type) << 6;         // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;       // Bits 5 y 4
    buffer[0] |= ((uint8_t)msg.dataType) << 2;     // Bits 3 y 2

    if (msg.dataType == DATA_TEMPERATURE) {
      buffer[0] |= ((uint8_t)msg.sign) << 1;       // Bit 1: Signo
    }
    // Asegurar que el bit 0 es cero
    buffer[0] &= 0xFE;

    // Los bits restantes son para los datos (16 bits)
    buffer[1] = (uint8_t)(msg.dataValue >> 8);     // Byte alto
    buffer[2] = (uint8_t)(msg.dataValue & 0xFF);   // Byte bajo
    bufferLength = 3; // Tres bytes en total
  } else {
    // Otros tipos de mensajes
  }

  // Depuración
  Serial.print(F("[Nodo][Debug] Mensaje empaquetado - buffer[0]: 0x"));
  Serial.print(buffer[0], HEX);
  Serial.print(F(", buffer[1]: 0x"));
  Serial.print(buffer[1], HEX);
  Serial.print(F(", buffer[2]: 0x"));
  Serial.print(buffer[2], HEX);
  Serial.print(F(", bufferLength: "));
  Serial.println(bufferLength);
}

void unpackMessage(const uint8_t* buffer, uint8_t bufferLength, Message& msg) {
  // Depuración
  Serial.print(F("[Nodo][Debug] buffer[0]: 0x"));
  Serial.print(buffer[0], HEX);
  Serial.print(F(", bufferLength: "));
  Serial.println(bufferLength);

  msg.type = (MessageType)(buffer[0] >> 6);
  Serial.print(F("[Nodo][Debug] msg.type: "));
  Serial.println((uint8_t)msg.type, BIN);

  if (msg.type == ACK_MSG) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11); // Bits 5 y 4
    msg.payloadLength = 0;
  } else if (msg.type == SCHEDULE) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11); // Bits 5 y 4
    msg.sf = (buffer[0] & 0b00000111);              // Bits 2,1,0
    msg.payloadLength = 0;
  } else if (msg.type == BEACON) {
    msg.payloadLength = 0;
  } else if (msg.type == DATA) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11);            // Bits 5 y 4
    msg.dataType = (DataMessageType)((buffer[0] >> 2) & 0b11); // Bits 3 y 2

    if (msg.dataType == DATA_TEMPERATURE) {
      msg.sign = (bool)((buffer[0] >> 1) & 0b1);              // Bit 1: Signo
    } else {
      msg.sign = false; // No se usa para presión
    }
    // Obtener el valor de los datos
    msg.dataValue = ((uint16_t)buffer[1] << 8) | buffer[2];
    msg.payloadLength = 0;
  } else {
    // Otros tipos de mensajes
  }
}

void setup() {
  Serial.begin(115200);

  Serial.print(F("[Nodo] Inicializando... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡Éxito!"));
  } else {
    Serial.print(F("Falló, código "));
    Serial.println(state);
    while (true) { delay(1000); }
  }

  radio.setFrequency(868.0);
  radio.setSpreadingFactor(12);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setCRC(true);
  radio.setSyncWord(0x12);

  radio.setPacketReceivedAction(setFlag);
  radio.setPacketSentAction(setFlag);

  Serial.println(F("[Nodo] Esperando BEACON..."));
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
    while (true) { delay(1000); }
  }
}

// Variables para el acceso por ranuras
bool scheduleReceived = false;
unsigned long slotTime = 5000; // Tiempo de ranura en milisegundos (5 segundos)
unsigned long dataSendTime = 0; // Momento en que el nodo debe enviar datos

void loop() {
  if (scheduleReceived && millis() >= dataSendTime && !operationDone) {
    // Es el turno del nodo para enviar datos
    sendData();
    scheduleReceived = false; // Resetear para esperar el próximo ciclo
  }

  if (operationDone) {
    operationDone = false;
    Serial.println(F("[Nodo] Operación completada."));

    if (operationState == RADIOLIB_ERR_NONE) {
      if (currentOperation == RADIO_RX) {
        Serial.println(F("[Nodo] Paquete recibido."));
        uint8_t buffer[256];
        int length = radio.getPacketLength();
        int state = radio.readData(buffer, length);

        if (state == RADIOLIB_ERR_NONE) {
          Message msg;
          unpackMessage(buffer, length, msg);

          if (msg.type == BEACON) {
            Serial.println(F("[Nodo][1] BEACON recibido, enviando ACK..."));
            sendAck();
          } else if (msg.type == SCHEDULE && msg.nodeId == nodeId) {
            Serial.println(F("[Nodo][3] SCHEDULE recibido."));
            uint8_t receivedSF = msg.sf + 5; // Decodificar SF (0 a 7) a SF real (5 a 12)
            Serial.print(F("[Nodo][3] SF asignado: SF"));
            Serial.println(receivedSF);

            // Comentamos el ajuste del SF para mantener SF12

            int state = radio.setSpreadingFactor(receivedSF);
            if (state == RADIOLIB_ERR_NONE) {
              Serial.println(F("[Nodo] SF ajustado correctamente."));
            } else {
              Serial.print(F("Falló al ajustar el SF, código "));
              Serial.println(state);
            }


            // Calcular el tiempo para enviar datos
            scheduleReceived = true;
            dataSendTime = millis() + (slotTime * (uint8_t)nodeId);
            Serial.print(F("[Nodo] Enviará datos en "));
            Serial.print((slotTime * (uint8_t)nodeId) / 1000);
            Serial.println(F(" segundos."));
          } else {
            Serial.println(F("[Nodo] Mensaje recibido no es relevante."));
          }
        } else {
          Serial.print(F("Falló al leer datos, código "));
          Serial.println(state);
        }

        operationState = radio.startReceive();
        currentOperation = RADIO_RX;
        Serial.println(F("[Nodo] Esperando mensajes..."));
        if (operationState != RADIOLIB_ERR_NONE) {
          Serial.print(F("Falló al iniciar recepción, código "));
          Serial.println(operationState);
        }
      }
    } else {
      Serial.print(F("Operación fallida, código "));
      Serial.println(operationState);
    }
  }
}

void sendAck() {
  Message msg;
  msg.type = ACK_MSG;
  msg.nodeId = nodeId;
  msg.payloadLength = 0;

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  Serial.println(F("[Nodo][2] Enviando ACK..."));
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo][2] ACK enviado con éxito."));
  } else {
    Serial.print(F("Falló al enviar ACK, código "));
    Serial.println(operationState);
  }

  Serial.println(F("[Nodo][2] Esperando mensajes..."));
  delay(500); // Retraso para evitar recibir la propia transmisión
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}

void sendData() {
  // Simulación de valores de temperatura y presión
  int16_t temperature = random(-20, 50); // Temperatura entre -20 y 50 °C
  uint16_t pressure = random(300, 1100); // Presión entre 300 y 1100 hPa

  sendTemperature(temperature);
  delay(1000); // Pequeña pausa entre envíos
  sendPressure(pressure);

  // Resetear SF al valor por defecto (SF12)
  int state = radio.setSpreadingFactor(12);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo] SF reseteado a SF12."));
  } else {
    Serial.print(F("Falló al resetear el SF, código "));
    Serial.println(state);
  }

  // Iniciar recepción nuevamente
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}


void sendTemperature(int16_t temperature) {
  Message msg;
  msg.type = DATA;
  msg.nodeId = nodeId;
  msg.dataType = DATA_TEMPERATURE;
  msg.sign = (temperature < 0) ? 1 : 0;
  msg.dataValue = abs(temperature); // Asegurarse de que el valor es positivo

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  Serial.println(F("[Nodo][4] Enviando temperatura..."));
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo][4] Temperatura enviada con éxito."));
  } else {
    Serial.print(F("[Nodo][4] Falló al enviar temperatura, código "));
    Serial.println(operationState);
  }

  delay(500); // Retraso para evitar recibir la propia transmisión

  // Iniciar recepción nuevamente
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}

void sendPressure(uint16_t pressure) {
  Message msg;
  msg.type = DATA;
  msg.nodeId = nodeId;
  msg.dataType = DATA_PRESSURE;
  msg.dataValue = pressure;

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  Serial.println(F("[Nodo][4] Enviando presión..."));
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo][4] Presión enviada con éxito."));
  } else {
    Serial.print(F("[Nodo][4] Falló al enviar presión, código "));
    Serial.println(operationState);
  }

  delay(500); // Retraso para evitar recibir la propia transmisión

  // Iniciar recepción nuevamente
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("[Nodo][4] Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}
