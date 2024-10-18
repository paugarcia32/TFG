#include <Arduino.h>
#include <RadioLib.h>

enum MessageType : uint8_t {
  BEACON = 0b00,
  ACK_MSG = 0b01,
  SCHEDULE = 0b10,
  DATA = 0b11
};

enum NodeID : uint8_t {
  NODE_0 = 0b00,
  NODE_1 = 0b01,
  NODE_2 = 0b10,
  NODE_3 = 0b11
};

struct Message {
  MessageType type;      
  NodeID nodeId;         
  uint8_t sf;            
  uint8_t payload[255];  
  uint8_t payloadLength; 
};

const NodeID nodeId = NODE_0; 

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

  // Para BEACON: solo 2 bits del tipo de mensaje
  if (msg.type == BEACON) {
    buffer[0] |= ((uint8_t)msg.type) << 6; // Bits 7 y 6
    bufferLength = 1; // Solo un byte
  }
  // Para ACK: 2 bits de tipo y 2 bits de ID
  else if (msg.type == ACK_MSG) {
    buffer[0] |= ((uint8_t)msg.type) << 6;       // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;     // Bits 5 y 4
    bufferLength = 1; // Solo un byte
  }
  // Para SCHEDULE: 2 bits de tipo, 2 bits de ID y 3 bits de SF
  else if (msg.type == SCHEDULE) {
    buffer[0] |= ((uint8_t)msg.type) << 6;       // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;     // Bits 5 y 4
    buffer[0] |= ((uint8_t)msg.sf) & 0b00000111; // Bits 2,1,0
    bufferLength = 1; // Solo un byte
  }
  // Otros tipos de mensajes con carga útil
  else {
    buffer[0] |= ((uint8_t)msg.type) << 6;
    memcpy(&buffer[1], msg.payload, msg.payloadLength);
    bufferLength = msg.payloadLength + 1;
  }
}

void unpackMessage(const uint8_t* buffer, uint8_t bufferLength, Message& msg) {
  // Extraer el tipo de mensaje de los bits 7 y 6
  msg.type = (MessageType)(buffer[0] >> 6);

  // Si es ACK, extraer el ID del nodo
  if (msg.type == ACK_MSG) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11); // Bits 5 y 4
    msg.payloadLength = 0;
  }
  // Si es SCHEDULE, extraer el ID y el SF
  else if (msg.type == SCHEDULE) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11); // Bits 5 y 4
    msg.sf = (buffer[0] & 0b00000111);              // Bits 2,1,0
    msg.payloadLength = 0;
  }
  // Si es BEACON, no hay carga útil
  else if (msg.type == BEACON) {
    msg.payloadLength = 0;
  }
  // Otros mensajes con carga útil
  else {
    msg.payloadLength = bufferLength - 1;
    memcpy(msg.payload, &buffer[1], msg.payloadLength);
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

void loop() {
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
            Serial.println(F("[Nodo] BEACON recibido, enviando ACK..."));
            sendAck();
          } else if (msg.type == SCHEDULE && msg.nodeId == nodeId) {
            Serial.println(F("[Nodo] SCHEDULE recibido."));
            uint8_t receivedSF = msg.sf + 5; // Decodificar SF (0 a 7) a SF real (5 a 12)
            Serial.print(F("[Nodo] SF asignado: SF"));
            Serial.println(receivedSF);

            int state = radio.setSpreadingFactor(receivedSF);
            if (state == RADIOLIB_ERR_NONE) {
              Serial.println(F("[Nodo] SF ajustado correctamente."));
            } else {
              Serial.print(F("Falló al ajustar el SF, código "));
              Serial.println(state);
            }
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
      } else if (currentOperation == RADIO_TX) {
        // Transmisión completada
        // Ya no es necesario manejar esto, ya que usamos transmisión bloqueante
      } else {
        // No debería suceder
        Serial.println(F("Operación completada desconocida"));
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

  Serial.println(F("[Nodo] Enviando ACK..."));
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo] ACK enviado con éxito."));
  } else {
    Serial.print(F("Falló al enviar ACK, código "));
    Serial.println(operationState);
  }

  Serial.println(F("[Nodo] Esperando mensajes..."));
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}
