#include <RadioLib.h>

SX1262 radio = new Module(8, 14, 12, 13);

int transmissionState = RADIOLIB_ERR_NONE;
int spreadingFactor = 12;
bool ackReceived = false;
volatile bool receivedFlag = false;

#define MAX_NODES 10
uint16_t nodeIDs[MAX_NODES];
uint8_t nodeCount = 0;
uint8_t nodeSFs[MAX_NODES];

void setFlag(void) {
  receivedFlag = true;
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
  radio.setSpreadingFactor(spreadingFactor);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setCRC(true);
  radio.setSyncWord(0x12);

  radio.setDio1Action(setFlag);
}

void loop() {
  enviarBeacon();
  esperarAck();

  if (ackReceived) {
    delay(500);
    enviarSchedule();
    ackReceived = false;
    nodeCount = 0; // Reiniciar la lista de nodos para la siguiente ronda
  }

  delay(30000);
}

void enviarBeacon() {
  Serial.print(F("[SX1262] Enviando beacon con SF"));
  Serial.print(spreadingFactor);
  Serial.println(F(" ..."));
  transmissionState = radio.transmit("BEACON");
  
  if (transmissionState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Beacon enviado con éxito!"));
  } else {
    Serial.print(F("Fallo, código "));
    Serial.println(transmissionState);
  }
}

void esperarAck() {
  // Limpiar bandera de recepción
  receivedFlag = false;

  // Iniciar recepción
  radio.startReceive();

  // Tiempo de espera en milisegundos
  unsigned long timeout = 10000; // Declaración de 'timeout'
  unsigned long startTime = millis();

  // Esperar hasta que se reciba un paquete o se alcance el tiempo de espera
  while (!receivedFlag && (millis() - startTime) < timeout) {
    // Esperar brevemente para evitar bloquear completamente el microcontrolador
    delay(10);
  }

  // Detener recepción y poner el radio en modo de espera
  radio.standby();

  if (receivedFlag) {
    receivedFlag = false;

    String receivedData;
    int state = radio.readData(receivedData);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[SX1262] Datos recibidos: "));
      Serial.println(receivedData);

      if (receivedData.startsWith("ACK:")) {
        String idStr = receivedData.substring(4);
        uint16_t nodeID = idStr.toInt();
        Serial.print(F("[SX1262] ACK recibido de nodo ID: "));
        Serial.println(nodeID);

        // Obtener RSSI y SNR
        float rssi = radio.getRSSI();
        float snr = radio.getSNR();
        Serial.print(F("[SX1262] RSSI: "));
        Serial.print(rssi);
        Serial.print(F(" dBm, SNR: "));
        Serial.print(snr);
        Serial.println(F(" dB"));

        // Calcular SF óptimo
        uint8_t optimalSF = calculateOptimalSF(rssi, snr);
        Serial.print(F("[SX1262] SF óptimo calculado: SF"));
        Serial.println(optimalSF);

        addNodeID(nodeID, optimalSF);
        ackReceived = true;
      } else {
        Serial.println(F("[SX1262] Datos inesperados recibidos en lugar de ACK."));
      }
    } else {
      Serial.print(F("[SX1262] Error al leer datos, código "));
      Serial.println(state);
    }
  } else {
    // No se recibió ningún paquete dentro del tiempo de espera
    Serial.println(F("[SX1262] ACK no recibido (tiempo de espera agotado)."));
  }
}

void enviarSchedule() {
  for (uint8_t i = 0; i < nodeCount; i++) {
    uint16_t nodeID = nodeIDs[i];
    uint8_t nodeSF = nodeSFs[i];
    unsigned long delayTime = (i + 1) * 3000; // Aumenta el delayTime para asegurar que el gateway esté listo

    String scheduleMessage = "SCHEDULE:" + String(nodeID) + ":" + String(delayTime) + ":" + String(nodeSF);

    Serial.print(F("[SX1262] Enviando programación a nodo "));
    Serial.print(nodeID);
    Serial.print(F(", SF: "));
    Serial.println(nodeSF);

    int state = radio.transmit(scheduleMessage);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("[SX1262] Programación enviada con éxito!"));
    } else {
      Serial.print(F("Fallo al enviar programación, código "));
      Serial.println(state);
    }
    // Puedes reducir o eliminar este retraso si tienes pocos nodos
    // delay(500); // Pequeño retraso entre transmisiones
  }

  // Iniciar recepción después de enviar todas las programaciones
  esperarDatos();
}

void esperarDatos() {
  for (uint8_t i = 0; i < nodeCount; i++) {
    uint16_t nodeID = nodeIDs[i];
    uint8_t nodeSF = nodeSFs[i];

    // Configurar el SF del gateway al SF del nodo
    radio.setSpreadingFactor(nodeSF);
    Serial.print(F("[SX1262] Configurando SF a "));
    Serial.println(nodeSF);

    // Limpiar bandera de recepción
    receivedFlag = false;

    // Iniciar recepción
    radio.startReceive();

    // Tiempo de espera en milisegundos (ajusta según sea necesario)
    unsigned long timeout = 5000;
    unsigned long startTime = millis();

    // Esperar hasta que se reciba un paquete o se alcance el tiempo de espera
    while (!receivedFlag && (millis() - startTime) < timeout) {
      delay(10);
    }

    // Detener recepción y poner el radio en modo de espera
    radio.standby();

    if (receivedFlag) {
      receivedFlag = false;

      String receivedData;
      int state = radio.readData(receivedData);

      if (state == RADIOLIB_ERR_NONE) {
        Serial.print(F("[SX1262] Datos recibidos: "));
        Serial.println(receivedData);

        // Procesar los datos recibidos
        if (receivedData.startsWith("DATA:")) {
          // Extraer el ID del nodo y los datos
          int idStart = 5;
          int idEnd = receivedData.indexOf(':', idStart);
          String idStr = receivedData.substring(idStart, idEnd);
          uint16_t receivedNodeID = idStr.toInt();

          if (receivedNodeID == nodeID) {
            String dataStr = receivedData.substring(idEnd + 1);

            Serial.print(F("[SX1262] Datos recibidos de nodo ID "));
            Serial.print(nodeID);
            Serial.print(F(": "));
            Serial.println(dataStr);
          } else {
            Serial.println(F("[SX1262] Datos recibidos de nodo inesperado."));
          }
        } else {
          Serial.println(F("[SX1262] Datos inesperados recibidos."));
        }
      } else {
        Serial.print(F("[SX1262] Error al leer datos, código "));
        Serial.println(state);
      }
    } else {
      Serial.print(F("[SX1262] No se recibieron datos del nodo ID "));
      Serial.println(nodeID);
    }
  }

  // Restablecer el SF del gateway a SF12 después de recibir datos
  radio.setSpreadingFactor(spreadingFactor);
  Serial.println(F("[SX1262] Spreading Factor del gateway reiniciado a 12."));
}


void addNodeID(uint16_t nodeID, uint8_t sf) {
  for (uint8_t i = 0; i < nodeCount; i++) {
    if (nodeIDs[i] == nodeID) {
      nodeSFs[i] = sf; // Actualizar SF si el ID ya existe
      return;
    }
  }
  if (nodeCount < MAX_NODES) {
    nodeIDs[nodeCount] = nodeID;
    nodeSFs[nodeCount] = sf;
    nodeCount++;
    Serial.print(F("[SX1262] Nodo añadido a la lista: "));
    Serial.println(nodeID);
  } else {
    Serial.println(F("[SX1262] Lista de nodos llena."));
  }
}

uint8_t calculateOptimalSF(float rssi, float snr) {
  if (rssi > -80 && snr > 10) {
    return 7;
  } else if (rssi > -90 && snr > 5) {
    return 8;
  } else if (rssi > -100 && snr > 0) {
    return 9;
  } else if (rssi > -110 && snr > -5) {
    return 10;
  } else if (rssi > -120 && snr > -10) {
    return 11;
  } else {
    return 12;
  }
}
