// /*
//    Nodo A (Transmisor) - SX126x + RadioLib
// */

// #include <RadioLib.h>

// #define NSS_PIN   8
// #define DIO1_PIN  14
// #define NRST_PIN  12
// #define BUSY_PIN  13


// SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);


// uint32_t packetCounter = 0;

// void setup() {
//   Serial.begin(115200);
//   while(!Serial);

//   Serial.println("[TX] Iniciando...");


//   int state = radio.begin(868.0, 125.0, 10, 5); 


//   if (state == RADIOLIB_ERR_NONE) {
//     Serial.println("[TX] Radio inicializada correctamente.");
//   } else {
//     Serial.print("[TX] Error al inicializar la radio. Código: ");
//     Serial.println(state);
//     while(true);
//   }


//   if (packetCounter = 0) {
//     delay(5000);
//   }
// }

// void loop() {
//   String outMessage = "Hola mundo #";
//   outMessage += packetCounter;

//   // Imprimir en Serial lo que vamos a enviar
//   Serial.print("[TX] Enviando -> ");
//   Serial.println(outMessage);
//   delay(50);

//   // Transmitir el paquete
//   int state = radio.transmit(outMessage);
//   if (state == RADIOLIB_ERR_NONE) {
//     // Éxito

//   } else {
//     // Error
//     Serial.print("[TX] Error al enviar. Código: ");
//     Serial.println(state);
//   }

//   // Aumentar contador de paquetes
//   packetCounter++;

//   // Esperar 2 segundos (ajusta el período a tus necesidades)
//   delay(5000);
// }

/*
   Nodo A (Transmisor) - SX126x + RadioLib
*/

#include <RadioLib.h>

#define NSS_PIN   8
#define DIO1_PIN  14
#define NRST_PIN  12
#define BUSY_PIN  13

SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);

uint32_t packetCounter = 0;
const uint32_t maxPackets = 20;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  Serial.println("[TX] Iniciando...");

  int state = radio.begin(868.0, 125.0, 7, 5); 

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[TX] Radio inicializada correctamente.");
  } else {
    Serial.print("[TX] Error al inicializar la radio. Código: ");
    Serial.println(state);
    while(true);
  }

    delay(5000);
}

void loop() {

  if (packetCounter >= maxPackets) {
    
    // Serial.println("[TX] Se enviaron todos los mensajes. Fin del programa.");
    while (true);
  }

  String outMessage = "Hola mundo #";
  outMessage += packetCounter;

  Serial.print("[TX] Enviando -> ");
  Serial.println(outMessage);
  delay(50);

  
  int state = radio.transmit(outMessage);
  if (state == RADIOLIB_ERR_NONE) {
    // Serial.println("[TX] Mensaje enviado correctamente.");
  } else {
    Serial.print("[TX] Error al enviar. Código: ");
    Serial.println(state);
  }

  
  packetCounter++;


  delay(5000);
}
