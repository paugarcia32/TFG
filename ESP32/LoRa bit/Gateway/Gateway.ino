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

struct NodeInfo {
  NodeID nodeId;
  uint8_t optimalSF;
};

#define MAX_NODES 4 
NodeInfo nodes[MAX_NODES];
uint8_t nodeCount = 0;


struct Message {
  MessageType type;      
  NodeID nodeId;         
  uint8_t sf;            
  uint8_t payload[255];  
  uint8_t payloadLength; 
};

volatile bool operationDone = false;
int operationState = RADIOLIB_ERR_NONE;

enum RadioOperation {
  RADIO_IDLE,
  RADIO_RX,
  RADIO_TX
};

RadioOperation currentOperation = RADIO_IDLE;

unsigned long startTime;
const unsigned long receiveDuration = 10000; 
bool scheduleSent = false;

SX1262 radio = new Module(8, 14, 12, 13);

uint8_t calculateOptimalSF(float rssi, float snr) {
  const int sensitivitySF[] = {-125, -127, -130, -132, -135, -137};
  const float snrLimit[] = {-7.5, -10.0, -12.5, -15.0, -17.5, -20.0};

  for (int i = 0; i < 6; i++) {
    if (rssi >= sensitivitySF[i] && snr >= snrLimit[i]) {
      return 7 + i;  
    }
  }
  return 12;
}

void packMessage(const Message& msg, uint8_t* buffer, uint8_t& bufferLength) {
  buffer[0] = 0;

  if (msg.type == BEACON) {
    buffer[0] |= ((uint8_t)msg.type) << 6; // Bits 7 y 6
    bufferLength = 1; // Solo un byte
  }
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

// Función de interrupción
#if defined(ESP8266) || defined(ESP32)
  IRAM_ATTR
#endif
void setFlag(void) {
  operationDone = true;
}

void setup() {
  Serial.begin(115200);

  Serial.print(F("[Gateway] Inicializando... "));
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

  sendBeacon();

  startTime = millis();
}

void loop() {
  if (!scheduleSent && (millis() - startTime >= receiveDuration)) {
    sendSchedules();
    scheduleSent = true;
    return;
  }

  if (operationDone) {
    operationDone = false;
    Serial.println(F("[Gateway] Operación completada."));

    if (operationState == RADIOLIB_ERR_NONE) {
      if (currentOperation == RADIO_TX) {
        Serial.println(F("[Gateway] BEACON enviado, esperando ACKs..."));
        operationState = radio.startReceive();
        currentOperation = RADIO_RX;
        Serial.println(F("[Gateway] Recepción iniciada."));
        if (operationState != RADIOLIB_ERR_NONE) {
          Serial.print(F("Falló al iniciar recepción, código "));
          Serial.println(operationState);
        }
      } else if (currentOperation == RADIO_RX) {
        Serial.println(F("[Gateway] Paquete recibido."));
        uint8_t buffer[256];
        int length = radio.getPacketLength();
        int state = radio.readData(buffer, length);

        if (state == RADIOLIB_ERR_NONE) {
          Message msg;
          unpackMessage(buffer, length, msg);

          if (msg.type == ACK_MSG) {
            Serial.println(F("[Gateway] Procesando ACK recibido."));
            Serial.print(F("[Gateway] Recibido ACK de Nodo ID: "));
            Serial.println((uint8_t)msg.nodeId, BIN);

            float rssi = radio.getRSSI();
            float snr = radio.getSNR();

            uint8_t optimalSF = calculateOptimalSF(rssi, snr);

            Serial.print(F("[Gateway] RSSI: "));
            Serial.print(rssi);
            Serial.println(F(" dBm"));

            Serial.print(F("[Gateway] SNR: "));
            Serial.print(snr);
            Serial.println(F(" dB"));

            Serial.print(F("[Gateway] SF óptimo para Nodo "));
            Serial.print((uint8_t)msg.nodeId, BIN);
            Serial.print(F(": SF"));
            Serial.println(optimalSF);

            bool nodeExists = false;
            for (uint8_t i = 0; i < nodeCount; i++) {
              if (nodes[i].nodeId == msg.nodeId) {
                nodeExists = true;
                nodes[i].optimalSF = optimalSF;
                break;
              }
            }
            if (!nodeExists && nodeCount < MAX_NODES) {
              nodes[nodeCount].nodeId = msg.nodeId;
              nodes[nodeCount].optimalSF = optimalSF;
              nodeCount++;
            }

          } else {
            Serial.println(F("[Gateway] Mensaje recibido no es ACK."));
          }
        } else {
          Serial.print(F("Falló al leer datos, código "));
          Serial.println(state);
        }

        operationState = radio.startReceive();
        currentOperation = RADIO_RX;
        Serial.println(F("[Gateway] Recepción iniciada."));
        if (operationState != RADIOLIB_ERR_NONE) {
          Serial.print(F("Falló al iniciar recepción, código "));
          Serial.println(operationState);
        }
      } else {
        Serial.println(F("Operación completada desconocida"));
      }
    } else {
      Serial.print(F("Operación fallida, código "));
      Serial.println(operationState);
    }
  }
}

void sendBeacon() {
  Message msg;
  msg.type = BEACON;
  msg.payloadLength = 0;

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  Serial.println(F("[Gateway] Enviando BEACON..."));
  operationState = radio.startTransmit(buffer, bufferLength);
  currentOperation = RADIO_TX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar transmisión, código "));
    Serial.println(operationState);
  }
}

void sendSchedules() {
  for (uint8_t i = 0; i < nodeCount; i++) {
    Message msg;
    msg.type = SCHEDULE;
    msg.nodeId = nodes[i].nodeId;
    // Codificar SF (5 a 12) en 3 bits (0 a 7)
    msg.sf = (nodes[i].optimalSF - 5) & 0b111;

    uint8_t buffer[256];
    uint8_t bufferLength;
    packMessage(msg, buffer, bufferLength);

    Serial.print(F("[Gateway] Enviando SCHEDULE a Nodo ID: "));
    Serial.println((uint8_t)msg.nodeId, BIN);

    operationState = radio.transmit(buffer, bufferLength);
    if (operationState == RADIOLIB_ERR_NONE) {
      Serial.println(F("[Gateway] SCHEDULE enviado."));
    } else {
      Serial.print(F("Falló al enviar SCHEDULE, código "));
      Serial.println(operationState);
    }
  }
}
