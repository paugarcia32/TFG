#include <Arduino.h>
#include <RadioLib.h>
#include "ProtocolDefinitions.h"
#include <RadioLib.h>


float frequency = 868.0;

SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);


NodeID thisNodeId = NODE_ID;  


// ----------------------------------------------------------------------------------
// Máquina de estados del Nodo
// ----------------------------------------------------------------------------------
enum NodeState {
  NODE_WAIT_BEACON,
  NODE_REQUEST_SENT,
  NODE_WAIT_SCHEDULE,
  NODE_WAIT_SLOT,
  NODE_SEND_DATA,
  NODE_SLEEP
};

// ----------------------------------------------------------------------------------
// Variables que guardaremos en RTC (persisten tras deep sleep)
// ----------------------------------------------------------------------------------
RTC_DATA_ATTR NodeState savedState         = NODE_WAIT_BEACON;  
RTC_DATA_ATTR uint8_t   savedSF            = 12;                
RTC_DATA_ATTR uint8_t   savedSlot          = 0xFF;              
RTC_DATA_ATTR bool      wokeFromDeepSleep  = false;             


RTC_DATA_ATTR uint32_t  sleepTimeMs        = 0;                 

// ----------------------------------------------------------------------------------
// Variables normales (no RTC)
// ----------------------------------------------------------------------------------

NodeState currentNodeState = NODE_WAIT_BEACON;


volatile bool packetReceived = false;


uint8_t assignedSF   = 12;    
uint8_t assignedSlot = 0xFF;  


unsigned long scheduleReceivedTime = 0;

// ----------------------------------------------------------------------------------
// Prototipos
// ----------------------------------------------------------------------------------
void nodeInitRadio();
void setFlag();
void startReceive();
// bool attemptChannelAccess(uint8_t maxRetries = 2, uint16_t randTime= 7000);
bool channelIsFree();
void sendRequest();
void handleBeacon(const Message& msg);
void handleSchedule(const SchedulePacket& sch);
void checkSlotTiming();
void sendData();

void goToDeepSleep(uint32_t ms);
void resumeFromDeepSleep();


void setup() {
  Serial.begin(115200);
  while (!Serial);


  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  Serial.print("[Nodo] Wakeup cause: ");
  Serial.println((int)wakeCause);



  nodeInitRadio();

if(wakeCause == ESP_SLEEP_WAKEUP_TIMER) {
    wokeFromDeepSleep = true;
    resumeFromDeepSleep();
  } else {
    wokeFromDeepSleep = false;
    // Reseteamos las variables RTC
    savedState  = NODE_WAIT_BEACON;
    savedSF     = 12;
    savedSlot   = 0xFF;
    sleepTimeMs = 0;

    // Estado inicial
    currentNodeState = NODE_WAIT_BEACON;
    assignedSF   = 12;
    assignedSlot = 0xFF;
    Serial.print("[Nodo] Iniciando... ID = 0x");
    Serial.println(thisNodeId, HEX);
    Serial.println("[Nodo] Arranque normal => Esperando Beacon...");
    startReceive();
  }
}

static unsigned long lastPrint = 0;
void loop() {

  
  // 1. Verificar si se recibió un paquete
  if (packetReceived) {
    packetReceived = false;

    int packetSize = radio.getPacketLength();
    if (packetSize == sizeof(Message)) {
      uint8_t buffer[sizeof(Message)];
      int state = radio.readData(buffer, packetSize);
      if (state == RADIOLIB_ERR_NONE) {
        Message msg;
        memcpy(&msg, buffer, sizeof(Message));

        
        if ((msg.header.type == BEACON) &&
            (msg.header.flag == GATEWAY_FLAG) &&
            (currentNodeState == NODE_WAIT_BEACON))
        {
          handleBeacon(msg);
        } else {
          Serial.println("[Nodo] Mensaje recibido, pero no es un Beacon esperado o estamos en otro estado.");
        }
      } else {
        Serial.print("[Nodo] Error al leer el paquete (Message). Code: ");
        Serial.println(state);
      }
    }
    else if (packetSize == sizeof(SchedulePacket)) {
      uint8_t buffer[sizeof(SchedulePacket)];
      int state = radio.readData(buffer, packetSize);
      if (state == RADIOLIB_ERR_NONE) {
        SchedulePacket sch;
        memcpy(&sch, buffer, sizeof(SchedulePacket));

        if ((sch.header.type == SCHEDULE) &&
            (sch.header.flag == GATEWAY_FLAG))
        {
          if (currentNodeState == NODE_REQUEST_SENT ||
              currentNodeState == NODE_WAIT_SCHEDULE)
          {
            handleSchedule(sch);
          }
        } else {
          Serial.println("[Nodo] Recibido un paquete que NO es SCHEDULE o no viene del Gateway.");
        }
      } else {
        Serial.print("[Nodo] Error al leer SCHEDULE. Code: ");
        Serial.println(state);
      }
    }
    else {
     
      Serial.print("[Nodo] Paquete recibido con tamano inesperado: ");
      Serial.println(packetSize);

      
      String str;
      int state = radio.readData(str);
      if (state == RADIOLIB_ERR_NONE) {
        Serial.print("[Nodo] Contenido en String: ");
        Serial.println(str);
      }
    }

   
    startReceive();
  }

  
  if (currentNodeState == NODE_WAIT_SLOT) {
    checkSlotTiming();
  } else if (currentNodeState == NODE_WAIT_BEACON) {
      if(millis() - lastPrint >= 2000) {
        Serial.println("[Nodo] (DEBUG) Sigo en WAIT_BEACON, esperando Beacon...");
        lastPrint = millis();
      }
    }

  delay(50);
}

// ----------------------------------------------------------------------------------
// Inicializa la radio
// ----------------------------------------------------------------------------------
void nodeInitRadio() {
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[Nodo] Radio inicializada correctamente!");
  } else {
    Serial.print("[Nodo] Error iniciando la radio. Codigo: ");
    Serial.println(state);
    while (true) { delay(1000); };
  }

  
  radio.setFrequency(frequency);
  radio.setSpreadingFactor(12);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setCRC(true);
  radio.setSyncWord(0x12);

 
  radio.setPacketReceivedAction(setFlag);
}

// ----------------------------------------------------------------------------------
// Callback que se llama cuando llega un nuevo paquete
// ----------------------------------------------------------------------------------
void setFlag() {
  packetReceived = true;
}

// ----------------------------------------------------------------------------------
// Función para habilitar el modo recepción (no bloqueante)
// ----------------------------------------------------------------------------------
void startReceive() {
  int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[Nodo] Error al poner en modo RX. Codigo: ");
    Serial.println(state);
  }
}

// ----------------------------------------------------------------------------------
// Manejar el Beacon recibido
// ----------------------------------------------------------------------------------
void handleBeacon(const Message& msg) {
  Serial.println("BRX");
  Serial.println("[Nodo] Beacon recibido correctamente!");


  bool canSend = attemptChannelAccess(2, 7000);
  if (canSend) {
    sendRequest();
    currentNodeState = NODE_REQUEST_SENT;
  } else {
    Serial.println("[Nodo] No se pudo acceder al canal. Regresamos a WAIT_BEACON...");
    currentNodeState = NODE_WAIT_BEACON;
  }
}

// ----------------------------------------------------------------------------------
// Manejar el SCHEDULE recibido 
// ----------------------------------------------------------------------------------
void handleSchedule(const SchedulePacket& sch) {
  Serial.println("[Nodo] Recibido SCHEDULE desde Gateway!");
  Serial.println("SRX");
  currentNodeState = NODE_WAIT_SCHEDULE; 

  bool found = false;
  for (uint8_t i = 0; i < sch.nodeCount; i++) {
    if (sch.nodeIds[i] == thisNodeId) {
      assignedSF = sch.sfs[i];
      assignedSlot = sch.slots[i];
      found = true;
      break;
    }
  }

  if (found) {
    Serial.print("[Nodo] Mi SF asignado = ");
    Serial.print(assignedSF);
    Serial.print(", Slot asignado = ");
    Serial.println(assignedSlot);

    radio.setSpreadingFactor(assignedSF);

    uint32_t slotDelayMs = 10000UL + (assignedSlot * 5000UL);

    savedState = NODE_WAIT_SLOT;  
    savedSF = assignedSF;
    savedSlot = assignedSlot;
    sleepTimeMs = slotDelayMs;  

    Serial.print("[Nodo] Entrando en deep sleep durante ");
    Serial.print(slotDelayMs);
    Serial.println("ms hasta mi slot...");

    goToDeepSleep(slotDelayMs);

  } else {

    Serial.println("[Nodo] No estoy en el SCHEDULE => Entraré en modo SLEEP hasta el próximo Beacon...");

 
    savedState = NODE_WAIT_BEACON;
    savedSF = 12;  
    savedSlot = 0xFF;  
    sleepTimeMs = 40000UL;  

    Serial.print("[Nodo] Entrando en deep sleep durante ");
    Serial.print(sleepTimeMs);
    Serial.println(" ms hasta el próximo Beacon...");

    goToDeepSleep(sleepTimeMs);
  }
}




void checkSlotTiming() {
  unsigned long now = millis();
  unsigned long elapsed = now - scheduleReceivedTime;


  uint32_t slotDelayMs = (assignedSlot + 1) * 5500UL; 

  if (elapsed >= slotDelayMs) {
    
    currentNodeState = NODE_SEND_DATA;
    sendData();
    
    currentNodeState = NODE_SLEEP;
  }
}



void sendData() {
  Serial.println("[Nodo] Es mi turno (slot). Envío DATA y calculo tiempo de sueño...");

  Message dataMsg;
  dataMsg.header.type     = DATA;
  dataMsg.header.flag     = NODE_FLAG;
  dataMsg.header.payload  = 0;
  dataMsg.header.nodeTxId = thisNodeId;
  dataMsg.header.nodeRxId = GATEWAY;

  dataMsg.sf        = assignedSF;
  dataMsg.dataValue = 0b11111111;  // Ejemplo de valor de datos

  uint8_t buffer[sizeof(Message)];
  memcpy(buffer, &dataMsg, sizeof(Message));

  bool canSend = attemptChannelAccess(1, 500);
  if (canSend){
    int state = radio.transmit(buffer, sizeof(Message));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[Nodo] DATA enviado exitosamente!");
    Serial.println("DTX");
  } else {
    Serial.print("[Nodo] Error enviando DATA. Código: ");
    Serial.println(state);
  }
  }
  

  uint32_t timeToSleepMs = 10000;  // 10s del tiempo del Gateway
  uint32_t remainingSlotsTime = (MAX_SLOTS - 1 - assignedSlot) * 5000UL;  

  timeToSleepMs += remainingSlotsTime;

  Serial.print("[Nodo] Tiempo total de sueño calculado: ");
  Serial.print(timeToSleepMs);
  Serial.println(" ms");

  savedSF = 12;  
  savedState = NODE_WAIT_BEACON;
  goToDeepSleep(timeToSleepMs);
}



// ----------------------------------------------------------------------------------
// Función para intentar acceder al canal un máximo de 'maxRetries' veces
// ----------------------------------------------------------------------------------
bool attemptChannelAccess(uint8_t maxRetries, uint16_t randTime) {
  for(uint8_t attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.print("[Nodo] Escaneando canal, intento ");
    Serial.print(attempt);
    Serial.println(" ...");

    if (channelIsFree()) {
      uint16_t randomWaitMs = random(0, randTime);
      Serial.print("[Nodo] Canal LIBRE, esperando ");
      Serial.print(randomWaitMs);
      Serial.println(" ms antes de enviar mensaje.");
      
      delay(randomWaitMs);  
      return true;
    } else {
      Serial.println("[Nodo] Canal OCUPADO, esperamos 1s y reintentamos...");
      delay(1000);
    }
  }
  return false;
}

// ----------------------------------------------------------------------------------
// Comprueba si el canal está libre (CAD)
// ----------------------------------------------------------------------------------
bool channelIsFree() {
  int cadState = radio.scanChannel();
  if (cadState == RADIOLIB_CHANNEL_FREE) {
    return true;
  } else if (cadState == RADIOLIB_LORA_DETECTED) {
    return false;
  } else {
    Serial.print("[Nodo] Error escaneando canal, code = ");
    Serial.println(cadState);
    return false;
  }
}

// ----------------------------------------------------------------------------------
// Envía un Request al Gateway 
// ----------------------------------------------------------------------------------
void sendRequest() {
  Serial.println("[Nodo] Enviando Request al Gateway...");

  Message reqMsg;
  reqMsg.header.type     = REQUEST;
  reqMsg.header.flag     = NODE_FLAG;
  reqMsg.header.payload  = 0;
  reqMsg.header.nodeTxId = thisNodeId;
  reqMsg.header.nodeRxId = GATEWAY;
  

  uint8_t buffer[sizeof(Message)];
  memcpy(buffer, &reqMsg, sizeof(Message));

  int state = radio.transmit(buffer, sizeof(Message));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[Nodo] Request enviado exitosamente!");
    Serial.println("RTX");
    currentNodeState = NODE_WAIT_SCHEDULE;
  } else {
    Serial.print("[Nodo] Error enviando Request. Codigo: ");
    Serial.println(state);
  }
}

void goToDeepSleep(uint32_t ms) {
  if(ms==0) ms=1000; 

  
  esp_sleep_enable_timer_wakeup((uint64_t)ms*1000ULL);
  Serial.println("D");
  
  Serial.println("[Nodo] *** Entrando a deep sleep ***");
  esp_deep_sleep_start();
}

void resumeFromDeepSleep() {

  Serial.println("[Nodo] => Reanudando tras deep sleep...");
   Serial.println("U");
  

  currentNodeState = savedState; 
  assignedSF   = savedSF;
  assignedSlot = savedSlot;

  Serial.print("   savedState=");
  Serial.println((int)savedState);
  Serial.print("   assignedSF=");
  Serial.println(assignedSF);
  Serial.print("   assignedSlot=");
  Serial.println(assignedSlot);
  Serial.print("   sleepTimeMs=");
  Serial.println(sleepTimeMs);

  
  if(currentNodeState == NODE_WAIT_SLOT) {
    currentNodeState = NODE_SEND_DATA;
    radio.setSpreadingFactor(assignedSF);

    sendData();
    currentNodeState = NODE_SLEEP;
    Serial.println("[Nodo] *** Data enviada tras deep sleep => FIN ***");
  } else if(savedState == NODE_WAIT_BEACON) {
    currentNodeState = NODE_WAIT_BEACON;
    startReceive();
  } else {
    
    Serial.println("[Nodo] No era WAIT_SLOT => no transmitimos. Revisar lógica...");
  }

  wokeFromDeepSleep = false;  


}
