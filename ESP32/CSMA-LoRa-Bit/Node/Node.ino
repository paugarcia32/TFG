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

struct Message {
  MessageType type;
  NodeID nodeId;
  uint8_t sf;
  DataMessageType dataType;
  bool sign;
  uint16_t dataValue;
  uint8_t payload[255];
  uint8_t payloadLength;
};

const NodeID nodeIdConst = NODE_0; // Cambia según tu nodo

SX1262 radio = new Module(8, 14, 12, 13);

volatile bool operationDone = false;
int operationState = RADIOLIB_ERR_NONE;

enum RadioOperation {
  RADIO_IDLE,
  RADIO_RX,
  RADIO_TX
};

RadioOperation currentOperation = RADIO_IDLE;

RTC_DATA_ATTR uint8_t savedMessageIndex = 0;
RTC_DATA_ATTR uint8_t savedMessageCount = 0;
RTC_DATA_ATTR uint8_t savedSpreadingFactor = 12;
RTC_DATA_ATTR bool continueDataSending = false;

bool scheduleReceived = false;
unsigned long slotTime = 5000;
unsigned long dataSendTime = 0;
uint8_t receivedSF = 12;

#if defined(ESP32)
  #include <esp_sleep.h>
#endif

#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void setFlag(void) {
  operationDone = true;
}

// Prototipos
void sendRequest();
void sendTemperature(int16_t temperature);
void sendPressure(uint16_t pressure);
void sendDataWithSF(uint8_t spreadingFactor);
void continueSendingData();
unsigned long calculateInterval(uint8_t spreadingFactor);
void packMessage(const Message& msg, uint8_t* buffer, uint8_t& bufferLength);
void unpackMessage(const uint8_t* buffer, uint8_t bufferLength, Message& msg);

void setup() {
  Serial.begin(115200);
  delay(1000);

  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  Serial.print("[Nodo] Causa del despertar: ");
  Serial.println((int)wakeCause);

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

  if (wakeCause == ESP_SLEEP_WAKEUP_TIMER && continueDataSending) {
    int s = radio.setSpreadingFactor(savedSpreadingFactor);
    if (s == RADIOLIB_ERR_NONE) {
      Serial.println(F("[Nodo] SF restaurado correctamente tras despertar."));
    } else {
      Serial.print(F("Falló al ajustar el SF al despertar, código "));
      Serial.println(s);
    }

    Serial.println(F("[Nodo] Continuando el envío de datos tras deep sleep..."));
    continueSendingData();
  } else {
    continueDataSending = false; 
    savedMessageIndex = 0;
    savedMessageCount = 0;
    savedSpreadingFactor = 12;

    Serial.println(F("[Nodo] Esperando BEACON..."));
    operationState = radio.startReceive();
    currentOperation = RADIO_RX;
    if (operationState != RADIOLIB_ERR_NONE) {
      Serial.print(F("Falló al iniciar recepción, código "));
      Serial.println(operationState);
      while (true) { delay(1000); }
    }
  }
}

void loop() {
  if (scheduleReceived && millis() >= dataSendTime && !operationDone && !continueDataSending) {
    Serial.print("[Nodo] Enviando datos con SF recibido: ");
    Serial.println(receivedSF);

    sendDataWithSF(receivedSF);
    scheduleReceived = false;
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
            Serial.println(F("[Nodo][1] BEACON recibido, enviando REQUEST..."));
            sendRequest();
          } else if (msg.type == SCHEDULE && msg.nodeId == nodeIdConst) {
            Serial.println(F("[Nodo][3] SCHEDULE recibido."));
            receivedSF = msg.sf + 5;
            Serial.print(F("[Nodo][3] SF asignado: SF"));
            Serial.println(receivedSF);

            int s = radio.setSpreadingFactor(receivedSF);
            if (s == RADIOLIB_ERR_NONE) {
              Serial.println(F("[Nodo] SF ajustado correctamente."));
            } else {
              Serial.print(F("Falló al ajustar el SF, código "));
              Serial.println(s);
            }

            scheduleReceived = true;
            dataSendTime = millis() + (slotTime * (uint8_t)nodeIdConst);
            Serial.print(F("[Nodo] Enviará datos en "));
            Serial.print((slotTime * (uint8_t)nodeIdConst) / 1000);
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

void sendRequest() {
  Message msg;
  msg.type = REQUEST;
  msg.nodeId = nodeIdConst;
  msg.payloadLength = 0;

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  Serial.println(F("[Nodo][2] Enviando REQUEST..."));
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo][2] REQUEST enviado con éxito."));
  } else {
    Serial.print(F("Falló al enviar REQUEST, código "));
    Serial.println(operationState);
  }

  Serial.println(F("[Nodo][2] Esperando mensajes..."));
  delay(2000);
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}

void sendDataWithSF(uint8_t spreadingFactor) {
  uint8_t messageCount;
  unsigned long interval;

  switch (spreadingFactor) {
    case 12: messageCount = 1; interval = 0; break;
    case 11: messageCount = 2; interval = 42000; break;
    case 10: messageCount = 4; interval = 21000; break;
    case 9:  messageCount = 4; interval = 10500; break;
    case 8:  messageCount = 4; interval = 5250;  break;
    case 7:  messageCount = 4; interval = 2125;  break;
    default: messageCount = 1; interval = 0;
  }

  savedMessageIndex = 0;
  savedMessageCount = messageCount;
  savedSpreadingFactor = spreadingFactor;
  continueDataSending = true; 

  for (uint8_t i = 0; i < messageCount; i++) {
    sendTemperature(random(-20, 50));
    sendPressure(random(300, 1100));

    if (i < messageCount - 1) {
      Serial.println(F("[Nodo] Entrando en deep sleep entre mensajes..."));
      savedMessageIndex = i + 1; 
      esp_sleep_enable_timer_wakeup(calculateInterval(spreadingFactor) * 1000ULL);
      esp_deep_sleep_start();
    }
  }

  continueDataSending = false;
  savedMessageIndex = 0;
  savedMessageCount = 0;
  savedSpreadingFactor = 12;

  int s = radio.setSpreadingFactor(12);
  if (s == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo] SF reseteado a SF12."));
  } else {
    Serial.print(F("[Nodo] Falló al resetear el SF, código "));
    Serial.println(s);
  }

  Serial.println(F("[Nodo] Reiniciando el flujo del loop..."));
}

void continueSendingData() {
  uint8_t startIndex = savedMessageIndex;
  uint8_t messageCount = savedMessageCount;
  uint8_t sf = savedSpreadingFactor;

  for (uint8_t i = startIndex; i < messageCount; i++) {
    sendTemperature(random(-20, 50));
    sendPressure(random(300, 1100));

    if (i < messageCount - 1) {
      Serial.println(F("[Nodo] Entrando en deep sleep entre mensajes..."));
      savedMessageIndex = i + 1;
      esp_sleep_enable_timer_wakeup(calculateInterval(sf) * 1000ULL);
      esp_deep_sleep_start();
    }
  }

  continueDataSending = false;
  savedMessageIndex = 0;
  savedMessageCount = 0;
  savedSpreadingFactor = 12;

  int s = radio.setSpreadingFactor(12);
  if (s == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo] SF reseteado a SF12."));
  } else {
    Serial.print(F("[Nodo] Falló al resetear el SF, código "));
    Serial.println(s);
  }

  Serial.println(F("[Nodo] Esperando BEACON..."));
  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}

unsigned long calculateInterval(uint8_t spreadingFactor) {
  switch (spreadingFactor) {
    case 11: return 42000;
    case 10: return 21000;
    case 9:  return 10500;
    case 8:  return 5250;
    case 7:  return 2125;
    default: return 0;
  }
}

// void sendTemperature(int16_t temperature) {
//   Message msg;
//   msg.type = DATA;
//   msg.nodeId = nodeIdConst;
//   msg.dataType = DATA_TEMPERATURE;
//   msg.sign = (temperature < 0);
//   msg.dataValue = abs(temperature);

//   uint8_t buffer[256];
//   uint8_t bufferLength;
//   packMessage(msg, buffer, bufferLength);

//   Serial.println(F("[Nodo][4] Enviando temperatura..."));
//   operationState = radio.transmit(buffer, bufferLength);
//   if (operationState == RADIOLIB_ERR_NONE) {
//     Serial.println(F("[Nodo][4] Temperatura enviada con éxito."));
//   } else {
//     Serial.print(F("[Nodo][4] Falló al enviar temperatura, código "));
//     Serial.println(operationState);
//   }

//   delay(2000);

//   operationState = radio.startReceive();
//   currentOperation = RADIO_RX;
//   if (operationState != RADIOLIB_ERR_NONE) {
//     Serial.print(F("Falló al iniciar recepción, código "));
//     Serial.println(operationState);
//   }
// }

// void sendPressure(uint16_t pressure) {
//   Message msg;
//   msg.type = DATA;
//   msg.nodeId = nodeIdConst;
//   msg.dataType = DATA_PRESSURE;
//   msg.dataValue = pressure;

//   uint8_t buffer[256];
//   uint8_t bufferLength;
//   packMessage(msg, buffer, bufferLength);

//   Serial.println(F("[Nodo][4] Enviando presión..."));
//   operationState = radio.transmit(buffer, bufferLength);
//   if (operationState == RADIOLIB_ERR_NONE) {
//     Serial.println(F("[Nodo][4] Presión enviada con éxito."));
//   } else {
//     Serial.print(F("[Nodo][4] Falló al enviar presión, código "));
//     Serial.println(operationState);
//   }

//   delay(2000);

//   operationState = radio.startReceive();
//   currentOperation = RADIO_RX;
//   if (operationState != RADIOLIB_ERR_NONE) {
//     Serial.print(F("[Nodo][4] Falló al iniciar recepción, código "));
//     Serial.println(operationState);
//   }
// }

void sendTemperature(int16_t temperature) {
  Message msg;
  msg.type = DATA;
  msg.nodeId = nodeIdConst;
  msg.dataType = DATA_TEMPERATURE;
  msg.sign = (temperature < 0);
  msg.dataValue = abs(temperature);

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  // Escanear canal antes de enviar
  while (true) {
    int scanResult = radio.scanChannel();
    if (scanResult == RADIOLIB_LORA_DETECTED) {
      Serial.println(F("[Nodo][Data] Canal ocupado, esperando 2s..."));
      delay(2000); 
    } else if (scanResult == RADIOLIB_CHANNEL_FREE) {
      Serial.println(F("[Nodo][Data] Canal libre, enviando temperatura..."));
      break;
    } else {
      Serial.print(F("[Nodo][Data] Falló al escanear canal, código "));
      Serial.println(scanResult);
      // Podrías implementar una lógica de reintentos o simplemente volver a intentar tras un delay
      delay(2000);
    }
  }

  // Ahora el canal está libre, proceder con la transmisión
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo][4] Temperatura enviada con éxito."));
  } else {
    Serial.print(F("[Nodo][4] Falló al enviar temperatura, código "));
    Serial.println(operationState);
  }

  delay(2000);

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
  msg.nodeId = nodeIdConst;
  msg.dataType = DATA_PRESSURE;
  msg.dataValue = pressure;

  uint8_t buffer[256];
  uint8_t bufferLength;
  packMessage(msg, buffer, bufferLength);

  // Escanear canal antes de enviar
  while (true) {
    int scanResult = radio.scanChannel();
    if (scanResult == RADIOLIB_LORA_DETECTED) {
      Serial.println(F("[Nodo][Data] Canal ocupado, esperando 2s..."));
      delay(2000);
    } else if (scanResult == RADIOLIB_CHANNEL_FREE) {
      Serial.println(F("[Nodo][Data] Canal libre, enviando presión..."));
      break;
    } else {
      Serial.print(F("[Nodo][Data] Falló al escanear canal, código "));
      Serial.println(scanResult);
      delay(2000);
    }
  }

  // Ahora el canal está libre, proceder con la transmisión
  operationState = radio.transmit(buffer, bufferLength);
  if (operationState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[Nodo][4] Presión enviada con éxito."));
  } else {
    Serial.print(F("[Nodo][4] Falló al enviar presión, código "));
    Serial.println(operationState);
  }

  delay(2000);

  operationState = radio.startReceive();
  currentOperation = RADIO_RX;
  if (operationState != RADIOLIB_ERR_NONE) {
    Serial.print(F("[Nodo][4] Falló al iniciar recepción, código "));
    Serial.println(operationState);
  }
}

void packMessage(const Message& msg, uint8_t* buffer, uint8_t& bufferLength) {
  buffer[0] = 0;

  if (msg.type == BEACON) {
    buffer[0] |= ((uint8_t)msg.type) << 6;
    bufferLength = 1;
  } else if (msg.type == REQUEST) {
    buffer[0] |= ((uint8_t)msg.type) << 6;
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;
    bufferLength = 1;
  } else if (msg.type == SCHEDULE) {
    buffer[0] |= ((uint8_t)msg.type) << 6;
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;
    buffer[0] |= (msg.sf & 0b00000111);
    bufferLength = 1;
  } else if (msg.type == DATA) {
    buffer[0] |= ((uint8_t)msg.type) << 6;
    buffer[0] |= ((uint8_t)msg.nodeId) << 4;
    buffer[0] |= ((uint8_t)msg.dataType) << 2;

    if (msg.dataType == DATA_TEMPERATURE) {
      buffer[0] |= ((uint8_t)msg.sign) << 1;
    }
    buffer[0] &= 0xFE;

    buffer[1] = (uint8_t)(msg.dataValue >> 8);
    buffer[2] = (uint8_t)(msg.dataValue & 0xFF);
    bufferLength = 3;
  } else {
    bufferLength = 0;
  }

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
  Serial.print(F("[Nodo][Debug] buffer[0]: 0x"));
  Serial.print(buffer[0], HEX);
  Serial.print(F(", bufferLength: "));
  Serial.println(bufferLength);

  msg.type = (MessageType)(buffer[0] >> 6);
  Serial.print(F("[Nodo][Debug] msg.type: "));
  Serial.println((uint8_t)msg.type, BIN);

  if (msg.type == REQUEST) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11);
    msg.payloadLength = 0;
  } else if (msg.type == SCHEDULE) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11);
    msg.sf = (buffer[0] & 0b00000111);
    msg.payloadLength = 0;
  } else if (msg.type == BEACON) {
    msg.payloadLength = 0;
  } else if (msg.type == DATA) {
    msg.nodeId = (NodeID)((buffer[0] >> 4) & 0b11);
    msg.dataType = (DataMessageType)((buffer[0] >> 2) & 0b11);

    if (msg.dataType == DATA_TEMPERATURE) {
      msg.sign = (bool)((buffer[0] >> 1) & 0b1);
    } else {
      msg.sign = false;
    }

    if (bufferLength >= 3) {
      msg.dataValue = ((uint16_t)buffer[1] << 8) | buffer[2];
    } else {
      msg.dataValue = 0;
    }
    msg.payloadLength = 0;
  }
}
