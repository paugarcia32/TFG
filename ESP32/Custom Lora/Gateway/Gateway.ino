#include <RadioLib.h>

SX1262 radio = new Module(8, 14, 12, 13);
volatile bool receivedFlag = false;
int currentSF = 12;

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
  radio.setSpreadingFactor(currentSF);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setCRC(true);
  // Eliminamos la línea de setHeaderType
  // radio.setHeaderType(RADIOLIB_SX126X_LORA_HEADER_EXPLICIT);
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

    uint8_t receivedData[256] = {0}; // Inicializar el buffer a ceros
    int state = radio.readData(receivedData, sizeof(receivedData) - 1);

    if (state == RADIOLIB_ERR_NONE) {
      size_t length = radio.getPacketLength();
      if (length >= sizeof(receivedData)) {
        length = sizeof(receivedData) - 1;
      }
      receivedData[length] = '\0';

      Serial.print(F("[SX1262] Paquete recibido: "));
      Serial.println((char*)receivedData);
      Serial.print(F("Longitud recibida: "));
      Serial.println(length);

      // Imprimir datos en hexadecimal
      Serial.print("Datos recibidos en hexadecimal: ");
      for (size_t i = 0; i < length; i++) {
        Serial.print(receivedData[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      if (strcmp((char*)receivedData, "BEACON") == 0) {
        enviarAck();
      } else if (strcmp((char*)receivedData, "NEGOTIATE_SF") == 0) {
        int optimalSF = calculateOptimalSF(radio.getRSSI(), radio.getSNR());
        Serial.print(F("[SX1262] Enviando SF propuesto: "));
        Serial.println(optimalSF);

        String sfMessage = String(optimalSF);
        radio.transmit(sfMessage);

        currentSF = optimalSF;
        radio.setSpreadingFactor(currentSF);
        Serial.print(F("[SX1262] Spreading Factor establecido a: "));
        Serial.println(currentSF);
      } else if (strcmp((char*)receivedData, "DATA") == 0) {
        Serial.println(F("[SX1262] Datos recibidos con éxito!"));

        currentSF = 12;
        radio.setSpreadingFactor(currentSF);
        Serial.println(F("[SX1262] Spreading Factor reiniciado a 12."));
      } else {
        Serial.println(F("[SX1262] Datos inesperados recibidos."));
      }

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
  Serial.println(F("[SX1262] Enviando ACK..."));
  String ackMessage = "ACK";
  int state = radio.transmit(ackMessage);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] ACK enviado con éxito!"));
    // Asegurar que la transmisión ha finalizado
    radio.finishTransmit();
    delay(500); // Aumentar el retraso a 500 ms
  } else {
    Serial.print(F("Fallo, código "));
    Serial.println(state);
  }

  // Configurar el radio para recibir
  radio.startReceive();
}

int calculateOptimalSF(float rssi, float snr) {
  if (rssi > -70 && snr > 10) {
    return 7; // SF7 para señales fuertes
  } else if (rssi > -80 && snr > 5) {
    return 9; // SF9 para señales medianas
  } else {
    return 12; // SF12 para señales débiles
  }
}
