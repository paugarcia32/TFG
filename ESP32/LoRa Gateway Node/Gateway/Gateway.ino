#include <RadioLib.h>

SX1262 radio = new Module(8, 14, 12, 13);

int transmissionState = RADIOLIB_ERR_NONE;
int spreadingFactor = 12;
bool ackReceived = false;
volatile bool receivedFlag = false;
unsigned long startTime;

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
    delay(500); // Esperar 500 ms
    // negociarSF(); // Si tienes esta función
    ackReceived = false;
  }

  delay(60000); // Enviar beacon cada 5 segundos
}

void enviarBeacon() {
  Serial.print(F("[SX1262] Enviando beacon con SF"));
  Serial.print(spreadingFactor);
  Serial.println(F(" ..."));
  transmissionState = radio.transmit("BEACON");
  
  if (transmissionState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Beacon enviado con éxito!"));
    // Iniciar recepción para esperar el ACK
    radio.startReceive();
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
  unsigned long timeout = 10000;
  unsigned long startTime = millis();

  // Esperar hasta que se reciba un paquete o se alcance el tiempo de espera
  while (!receivedFlag && (millis() - startTime) < timeout) {
    // Esperar brevemente para evitar bloquear completamente el microcontrolador
    delay(10);
  }

  // Detener recepción y poner el radio en modo de espera
  radio.standby();

  if (receivedFlag) {
    // Se recibió un paquete
    receivedFlag = false;

    String receivedData;
    int state = radio.readData(receivedData);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[SX1262] Datos recibidos: "));
      Serial.println(receivedData);

      if (receivedData == "ACK") {
        Serial.println(F("[SX1262] ACK recibido!"));
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
    Serial.println(F("[SX1262] ACK no recibido (tiempo de espera agotado). Reintentando..."));
  }
}

void enviarDatos() {
  Serial.println(F("[SX1262] Enviando datos con nuevo SF..."));
  transmissionState = radio.transmit("DATA");

  if (transmissionState == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Datos enviados con éxito!"));
  } else {
    Serial.print(F("Fallo, código "));
    Serial.println(transmissionState);
  }
}
