#include <Arduino.h>
#include <RadioLib.h>

enum MessageType : uint8_t {
  BEACON = 0b00,
  REQUEST = 0b01,
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

struct NodeInfo {
  NodeID nodeId;
  uint8_t optimalSF;
};

#define MAX_NODES 4 
NodeInfo nodes[MAX_NODES];
uint8_t nodeCount = 0;

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

volatile bool operationDone = false;
int operationState = RADIOLIB_ERR_NONE;

enum RadioOperation {
  RADIO_IDLE,
  RADIO_RX,
  RADIO_TX
};

RadioOperation currentOperation = RADIO_IDLE;

unsigned long startTime;
const unsigned long cycleDuration = 30000; 
unsigned long cycleStartTime;
bool scheduleSent = false;


const unsigned long slotTime = 85000; 

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
  else if (msg.type == REQUEST) {
    buffer[0] |= ((uint8_t)msg.type) << 6;       // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;     // Bits 5 y 4
    bufferLength = 1; // Solo un byte
  }
  else if (msg.type == SCHEDULE) {
    buffer[0] |= ((uint8_t)msg.type) << 6;       // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;     // Bits 5 y 4
    buffer[0] |= ((uint8_t)msg.sf) & 0b00000111; // Bits 2,1,0
    bufferLength = 1; // Solo un byte
  }
  else if (msg.type == DATA) {
    buffer[0] |= ((uint8_t)msg.type) << 6;         // Bits 7 y 6
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;       // Bits 5 y 4
    buffer[0] |= ((uint8_t)msg.dataType) << 2;     // Bits 3 y 2

    if (msg.dataType == DATA_TEMPERATURE) {
      buffer[0] |= ((uint8_t)msg.sign) << 1;       // Bit 1: 
    }
    
    buffer[0] &= 0xFE;


    buffer[1] = (uint8_t)(msg.dataValue >> 8);     
    buffer[2] = (uint8_t)(msg.dataValue & 0xFF);   
    bufferLength = 3; 
  } else {
    
  }
}

void unpackMessage(const uint8_t* buffer, uint8_t bufferLength, Message& msg) {
  Serial.print(F("[Gateway][Debug] buffer[0]: 0x"));
  Serial.print(buffer[0], HEX);
  Serial.print(F(", bufferLength: "));
  Serial.println(bufferLength);

  msg.type = (MessageType)(buffer[0] >> 6);
  Serial.print(F("[Gateway][Debug] msg.type: "));
  Serial.println((uint8_t)msg.type, BIN);

  if (msg.type == REQUEST) {
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
      msg.sign = false;
    }
    // Obtener el valor de los datos
    msg.dataValue = ((uint16_t)buffer[1] << 8) | buffer[2];
    msg.payloadLength = 0;
  } else {
    
  }
}

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

  cycleStartTime = millis();
  sendBeacon();
}

void loop() {
  unsigned long currentTime = millis();

  if ((currentTime - cycleStartTime) >= cycleDuration) {
    // Reiniciar el ciclo
    nodeCount = 0;
    scheduleSent = false;
    cycleStartTime = currentTime;
    sendBeacon();
    return;
  }

  if (!scheduleSent && (currentTime - cycleStartTime >= 10000)) {

    sendSchedules();
    scheduleSent = true;

    receiveDataFromNodes();
    return;
  }

  if (operationDone) {
    operationDone = false;
    Serial.println(F("[Gateway] Operación completada."));

    if (operationState == RADIOLIB_ERR_NONE) {
      if (currentOperation == RADIO_TX) {
        Serial.println(F("[Gateway] Transmisión completada."));
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
          
          Serial.print(F("[Gateway][Debug] Datos recibidos - buffer[0]: 0x"));
          Serial.print(buffer[0], HEX);
          if (length > 1) {
            Serial.print(F(", buffer[1]: 0x"));
            Serial.print(buffer[1], HEX);
          }
          if (length > 2) {
            Serial.print(F(", buffer[2]: 0x"));
            Serial.print(buffer[2], HEX);
          }
          Serial.print(F(", length: "));
          Serial.println(length);

          Message msg;
          unpackMessage(buffer, length, msg);


          if (msg.type == REQUEST) {
            Serial.println(F("[Gateway] Procesando REQUEST recibido."));
            Serial.print(F("[Gateway] [2] Recibido REQUEST de Nodo ID: "));
            Serial.println((uint8_t)msg.nodeId, BIN);

            float rssi = radio.getRSSI();
            float snr = radio.getSNR();

            uint8_t optimalSF = calculateOptimalSF(rssi, snr);

            Serial.print(F("[Gateway][2] RSSI: "));
            Serial.print(rssi);
            Serial.println(F(" dBm"));

            Serial.print(F("[Gateway][2] SNR: "));
            Serial.print(snr);
            Serial.println(F(" dB"));

            Serial.print(F("[Gateway][2] SF óptimo para Nodo "));
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
          } else if (msg.type == DATA) {
            Serial.println(F("[Gateway][4] Mensaje de datos recibido."));
            Serial.print(F("[Gateway][4] Nodo ID: "));
            Serial.println((uint8_t)msg.nodeId, BIN);
            if (msg.dataType == DATA_TEMPERATURE) {
              int16_t temperature = msg.dataValue;
              if (msg.sign) {
                temperature = -temperature;
              }
              Serial.print(F("[Gateway][4.1] Temperatura recibida: "));
              Serial.print(temperature);
              Serial.println(F(" °C"));
            } else if (msg.dataType == DATA_PRESSURE) {
              uint16_t pressure = msg.dataValue;
              Serial.print(F("[Gateway][4.2] Presión recibida: "));
              Serial.print(pressure);
              Serial.println(F(" hPa"));
            }
          } else {
            Serial.println(F("[Gateway][404] Mensaje recibido no es REQUEST ni DATA."));
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

  Serial.println(F("[Gateway][1] Enviando BEACON..."));
  operationState = radio.startTransmit(buffer, bufferLength);
  currentOperation = RADIO_TX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("[Gateway][1] Falló al iniciar transmisión, código "));
    Serial.println(operationState);
  }
}

void sendSchedules() {
  for (uint8_t i = 0; i < nodeCount; i++) {
    Message msg;
    msg.type = SCHEDULE;
    msg.nodeId = nodes[i].nodeId;

    msg.sf = (nodes[i].optimalSF - 5) & 0b111;

    uint8_t buffer[256];
    uint8_t bufferLength;
    packMessage(msg, buffer, bufferLength);

    Serial.print(F("[Gateway][3] Enviando SCHEDULE a Nodo ID: "));
    Serial.println((uint8_t)msg.nodeId, BIN);

    // int state = radio.setSpreadingFactor(nodes[i].optimalSF);
    // if (state == RADIOLIB_ERR_NONE) {
    //   Serial.print(F("[Gateway] SF ajustado a SF"));
    //   Serial.println(nodes[i].optimalSF);
    // } else {
    //   Serial.print(F("Falló al ajustar el SF, código "));
    //   Serial.println(state);
    // }

    operationState = radio.transmit(buffer, bufferLength);
    currentOperation = RADIO_TX;
    if (operationState == RADIOLIB_ERR_NONE) {
      Serial.println(F("[Gateway][3] SCHEDULE enviado."));
    } else {
      Serial.print(F("[Gateway][3] Falló al enviar SCHEDULE, código "));
      Serial.println(operationState);
    }

    delay(100);
  }

  receiveDataFromNodes();
}



void receiveDataFromNodes() {
  for (uint8_t i = 0; i < nodeCount; i++) {
    NodeInfo node = nodes[i];
    uint8_t nodeSF = node.optimalSF;

    int state = radio.setSpreadingFactor(nodeSF);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[Gateway] SF ajustado a SF"));
      Serial.println(nodeSF);
    } else {
      Serial.print(F("Falló al ajustar el SF, código "));
      Serial.println(state);
    }

    unsigned long slotStartTime = millis();
    unsigned long slotEndTime = slotStartTime + slotTime;

    Serial.print(F("[Gateway] Escuchando datos del nodo "));
    Serial.print((uint8_t)node.nodeId);
    Serial.print(F(" durante "));
    Serial.print(slotTime / 1000);
    Serial.println(F(" segundos."));

    // Mantener la recepción activa durante toda la ranura
    operationState = radio.startReceive();
    currentOperation = RADIO_RX;
    if (operationState != RADIOLIB_ERR_NONE) {
      Serial.print(F("Falló al iniciar recepción, código "));
      Serial.println(operationState);
    }

    while (millis() < slotEndTime) {
      if (operationDone) {
        operationDone = false;

        if (operationState == RADIOLIB_ERR_NONE && currentOperation == RADIO_RX) {
          uint8_t buffer[256];
          int length = radio.getPacketLength();
          int state = radio.readData(buffer, length);

          if (state == RADIOLIB_ERR_NONE) {
            Message msg;
            unpackMessage(buffer, length, msg);

            if (msg.type == DATA && msg.nodeId == node.nodeId) {
              Serial.println(F("[Gateway] Datos recibidos del nodo."));

              if (msg.dataType == DATA_TEMPERATURE) {
                int16_t temperature = msg.dataValue;
                if (msg.sign) {
                  temperature = -temperature;
                }
                Serial.print(F("[Gateway][4.1] Temperatura recibida: "));
                Serial.print(temperature);
                Serial.println(F(" °C"));
              } else if (msg.dataType == DATA_PRESSURE) {
                uint16_t pressure = msg.dataValue;
                Serial.print(F("[Gateway][4.2] Presión recibida: "));
                Serial.print(pressure);
                Serial.println(F(" hPa"));
              }
            } else {
              Serial.println(F("[Gateway] Mensaje recibido no es de datos del nodo esperado."));
            }
          } else {
            Serial.print(F("Falló al leer datos, código "));
            Serial.println(state);
          }

          // Reiniciar recepción para el siguiente mensaje
          operationState = radio.startReceive();
          currentOperation = RADIO_RX;
          if (operationState != RADIOLIB_ERR_NONE) {
            Serial.print(F("Falló al iniciar recepción, código "));
            Serial.println(operationState);
          }
        }
      }
    }

    Serial.println(F("[Gateway] Finalizó el tiempo de ranura para el nodo."));

    // Restablecer SF a 12
    state = radio.setSpreadingFactor(12);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("[Gateway] SF reseteado a SF12."));
    } else {
      Serial.print(F("Falló al resetear el SF, código "));
      Serial.println(state);
    }
  }
}


