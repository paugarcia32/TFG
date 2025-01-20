#include <Arduino.h>
#include <RadioLib.h>
#include "ProtocolDefinitions.h"

// --- BLE, WiFi, NVS, HTTP ---
#include <ArduinoBLE.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>


float frequency = 868.0;

// Objeto RadioLib
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);

// ID del nodo LoRa
NodeID thisNodeId = NODE_ID; // Ajustar si lo deseas

// --------------------------------------------------------------------------------
// Modo global y estados
// --------------------------------------------------------------------------------
enum GlobalMode {
  MODE_WIFI,
  MODE_LORA
};

enum NodeState {
  NODE_WAIT_BEACON,
  NODE_REQUEST_SENT,
  NODE_WAIT_SCHEDULE,
  NODE_WAIT_SLOT,
  NODE_SEND_DATA,
  NODE_SLEEP
};

// --------------------------------------------------------------------------------
// Variables que persisten con deep_sleep (RTC_DATA_ATTR)
// --------------------------------------------------------------------------------
RTC_DATA_ATTR NodeState savedState       = NODE_WAIT_BEACON;
RTC_DATA_ATTR uint8_t   savedSF          = 12;
RTC_DATA_ATTR uint8_t   savedSlot        = 0xFF;
RTC_DATA_ATTR uint32_t  sleepTimeMs      = 0;

// --------------------------------------------------------------------------------
// Variables globales
// --------------------------------------------------------------------------------
GlobalMode  gMode            = MODE_LORA; // Se determinará en setup()
NodeState   currentNodeState = NODE_WAIT_BEACON;
volatile bool packetReceived  = false;
uint8_t     assignedSF       = 12;
uint8_t     assignedSlot     = 0xFF;

unsigned long scheduleReceivedTime = 0;

bool wifiConfigured = false; 

// BLE & WiFi
Preferences preferences;
BLEService wifiConfigService("0x008D");
BLEStringCharacteristic wifiCredentialsCharacteristic(
  "c6e0bf23-f27d-4e1a-9ed8-fdfbd1b81e33",
  BLERead | BLEWrite | BLEEncryption,
  128
);

bool wifiConnected = false;

// Ventanas / Intervalos
const unsigned long BLE_CONFIG_WINDOW_MS   = 30000; // 30s config BLE
const unsigned long WIFI_SEND_INTERVAL_MS  = 60000; // 60s entre envíos WiFi
unsigned long bleConfigStartTime = 0;

// --------------------------------------------------------------------------------
// Prototipos
// --------------------------------------------------------------------------------
void tryConnectWiFiFromNVS();
void initBLE();
void onDataReceived(BLEDevice central, BLECharacteristic characteristic);

void startLoRa();
void nodeInitRadio();
void setFlag();
void startReceive();

bool attemptChannelAccess(uint8_t maxRetries, uint16_t randTime);
bool channelIsFree();
void sendRequest();
void handleBeacon(const Message& msg);
void handleSchedule(const SchedulePacket& sch);
void checkSlotTiming();
void sendData();

void sendDataToServerOverWiFi();

void goToDeepSleep(uint32_t ms);
void resumeFromDeepSleep();

// --------------------------------------------------------------------------------
// setup()
// --------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[Nodo] *** START ***");

  preferences.begin("wifiCreds", false);

  // 1) Leemos el flag "wifiConfigured"
  wifiConfigured = preferences.getBool("wifiConfigured", false);

  // 2) Si NO está configurado => intentamos configurar + BLE
  if(!wifiConfigured) {
    Serial.println("[Nodo] => configDone=FALSE => Chequeo si hay SSID/Password en NVS...");
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("password", "");

    if(ssid == "") {
      // 2.a) No hay credenciales => abrimos ventana BLE
      Serial.println("[Nodo] No hay credenciales => abrimos BLE 30s");
      initBLE();

      unsigned long bleStart = millis();
      const unsigned long BLE_WINDOW_MS = 30000;

      while((millis() - bleStart < BLE_WINDOW_MS) && !wifiConnected) {
        BLEDevice central = BLE.central();
        if(central) {
          while(central.connected()) {
            BLE.poll();
            // si en 'onDataReceived()' se conecta a WiFi => wifiConnected = true
            if(wifiConnected) break;
            if(millis() - bleStart >= BLE_WINDOW_MS) {
              central.disconnect();
              break;
            }
            delay(10);
          }
        }
        delay(50);
      }
      BLE.end();

      // al terminar la ventana BLE, si wifiConnected se puso en true => ya hay SSID
      if(wifiConnected) {
        // Marcamos configDone=true para no repetir BLE en siguientes arranques
        preferences.putBool("wifiConfigured", true);
        wifiConfigured = true;
        Serial.println("[Nodo] => WiFi configurado y conectado (primera vez).");
      } else {
        Serial.println("[Nodo] => No se configuró nada => MODO LoRa.");
      }
    } else {
      // 2.b) Hay credenciales en NVS, pero aún no marcamos configDone => las probamos
      Serial.println("[Nodo] => Existe SSID en NVS, probamos conectar...");
      WiFi.begin(ssid.c_str(), pass.c_str());

      int tries = 20;
      while(WiFi.status() != WL_CONNECTED && tries>0) {
        delay(500); Serial.print(".");
        tries--;
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Nodo] WiFi conectado => configDone=true");
        preferences.putBool("wifiConfigured", true);
        wifiConfigured = true;
      } else {
        Serial.println("\n[Nodo] Falló conexión => pasamos a LoRa");
      }
    }
  } // fin de "if(!wifiConfigured)"

  // 3) Decidimos modo final
  if(wifiConfigured) {
    // Si quieres seguir usando WiFi en cada arranque,
    // puedes quedarte en WiFi y mandar datos, etc.
    Serial.println("[Nodo] => MODO WIFI (porque configDone=true).");
    sendDataToServerOverWiFi();
    goToDeepSleep(60000); // duerme 60s
  } else {
    Serial.println("[Nodo] => MODO LoRa");
    startLoRa();
  }
}

// --------------------------------------------------------------------------------
// loop()
// --------------------------------------------------------------------------------
void loop() {
  // --- MODO WIFI ---
  if (gMode == MODE_WIFI) {
    static bool sentOnce = false;
    if (!sentOnce) {
      Serial.println("[Nodo] (WIFI) Envío datos y duermo 1 min...");
      sendDataToServerOverWiFi();
      goToDeepSleep(WIFI_SEND_INTERVAL_MS);
      sentOnce = true;
    }
    // Al despertar, se repite el setup(), se reintenta WiFi, etc.
  }

  // --- MODO LORA ---
  else {
    // Procesamos recepción de paquetes
    if (packetReceived) {
      packetReceived = false;
      int packetSize = radio.getPacketLength();
      if (packetSize == sizeof(Message)) {
        uint8_t buffer[sizeof(Message)];
        int state = radio.readData(buffer, packetSize);
        if (state == RADIOLIB_ERR_NONE) {
          Message msg;
          memcpy(&msg, buffer, sizeof(Message));

          if (msg.header.type == BEACON &&
              msg.header.flag == GATEWAY_FLAG &&
              currentNodeState == NODE_WAIT_BEACON) 
          {
            handleBeacon(msg);
          } else {
            Serial.println("[Nodo] Mensaje no esperado o estado distinto.");
          }
        }
      } 
      else if (packetSize == sizeof(SchedulePacket)) {
        uint8_t buffer[sizeof(SchedulePacket)];
        int state = radio.readData(buffer, packetSize);
        if (state == RADIOLIB_ERR_NONE) {
          SchedulePacket sch;
          memcpy(&sch, buffer, sizeof(SchedulePacket));
          if (sch.header.type == SCHEDULE &&
              sch.header.flag == GATEWAY_FLAG &&
             (currentNodeState == NODE_REQUEST_SENT ||
              currentNodeState == NODE_WAIT_SCHEDULE))
          {
            handleSchedule(sch);
          }
        }
      } else {
        Serial.print("[Nodo] Paquete tamaño inesperado: ");
        Serial.println(packetSize);
        String dummy;
        radio.readData(dummy); // limpiar
      }
      startReceive();
    }

    // Revisión de estados
    if (currentNodeState == NODE_WAIT_SLOT) {
      checkSlotTiming(); // Ver si ya es hora de enviar
    }
    else if (currentNodeState == NODE_WAIT_BEACON) {
      // Solo ejemplo de impresión ocasional
      static unsigned long lastPrint = 0;
      if (millis() - lastPrint >= 2000) {
        Serial.println("[Nodo] (LORA) Esperando Beacon...");
        lastPrint = millis();
      }
    }

    delay(50);
  }
}

// --------------------------------------------------------------------------------
// BLE: Inicialización + Callback
// --------------------------------------------------------------------------------
void initBLE() {
  if (!BLE.begin()) {
    Serial.println("[Nodo] Error iniciando BLE!");
    return;
  }
  Serial.println("[Nodo] BLE iniciado.");

  BLE.setDeviceName("Nodo-Config");
  BLE.setLocalName("Nodo-Config");
  BLE.setAdvertisedService(wifiConfigService);

  wifiConfigService.addCharacteristic(wifiCredentialsCharacteristic);
  BLE.addService(wifiConfigService);

  // Callback al escribir la característica
  wifiCredentialsCharacteristic.setEventHandler(BLEWritten, onDataReceived);

  BLE.advertise();
  Serial.println("[Nodo] Anunciando servicio BLE...");
}

void onDataReceived(BLEDevice central, BLECharacteristic characteristic) {
  // Leer "SSID:Password"
  int length = characteristic.valueLength();
  const uint8_t* data = characteristic.value();
  char buffer[length+1];
  memcpy(buffer, data, length);
  buffer[length] = '\0';

  String receivedData = String(buffer);
  int sep = receivedData.indexOf(':');
  if (sep > 0) {
    String ssid = receivedData.substring(0, sep);
    String pass = receivedData.substring(sep+1);

    // Guardar en NVS
    preferences.putString("ssid", ssid);
    preferences.putString("password", pass);

    // Intentar conectar enseguida
    WiFi.begin(ssid.c_str(), pass.c_str());
    int tries = 20;
    while (WiFi.status() != WL_CONNECTED && tries>0) {
      delay(500);
      Serial.print(".");
      tries--;
    }
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi conectado!");
      wifiConnected = true;
    } else {
      Serial.println("\nNo se pudo conectar WiFi");
    }
  }
}


// --------------------------------------------------------------------------------
// MODO WIFI: Enviar JSON al servidor
// --------------------------------------------------------------------------------
void sendDataToServerOverWiFi() {
  if (!wifiConnected) {
    Serial.println("[Nodo] No hay WiFi => no envío nada.");
    return;
  }
  HTTPClient http;
  const char* serverUrl = "http://192.168.1.40:3000/data"; // Ajustar a tu endpoint

  Serial.println("[Nodo] Envío JSON al servidor...");

  // Ejemplo simple
  float temperature = 25.5;
  String json = "[{\"nodeId\":\"0x";
  json += String(thisNodeId, HEX);
  json += "\",\"temperature\":";
  json += temperature;
  json += "}]";

  Serial.print("[Nodo] JSON: ");
  Serial.println(json);

  http.begin(serverUrl);
  http.addHeader("Content-Type","application/json");
  int httpResponseCode = http.POST(json);

  if (httpResponseCode > 0) {
    Serial.printf("[Nodo] POST code: %d\n", httpResponseCode);
    if (httpResponseCode == 200 || httpResponseCode == 201) {
      String payload = http.getString();
      Serial.println("[Nodo] Respuesta del servidor: " + payload);
    }
  } else {
    Serial.printf("[Nodo] Error POST: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

// --------------------------------------------------------------------------------
// MODO LORA
// --------------------------------------------------------------------------------
void startLoRa() {
  nodeInitRadio();

  // Vemos si venimos de deep_sleep
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  if (wakeCause == ESP_SLEEP_WAKEUP_TIMER) {
    // Retomamos donde estábamos
    resumeFromDeepSleep();
  } else {
    // Arranque "limpio"
    savedState  = NODE_WAIT_BEACON;
    savedSF     = 12;
    savedSlot   = 0xFF;
    sleepTimeMs = 0;

    currentNodeState = NODE_WAIT_BEACON;
    assignedSF       = 12;
    assignedSlot     = 0xFF;

    Serial.println("[Nodo] => MODO LORA => Esperando Beacon...");
    startReceive();
  }
}

void nodeInitRadio() {
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[Nodo] Radio inicializada!");
  } else {
    Serial.print("[Nodo] Error iniciando Radio. Codigo: ");
    Serial.println(state);
    while (true) { delay(100); }
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

void setFlag() {
  packetReceived = true;
}

void startReceive() {
  int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[Nodo] Error al poner en RX. Code: ");
    Serial.println(state);
  }
}

// --------------------------------------------------------------------------------
// Cadena de acceso al canal (CAD) para evitar colisiones
// --------------------------------------------------------------------------------
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

bool channelIsFree() {
  int cadState = radio.scanChannel();
  if (cadState == RADIOLIB_CHANNEL_FREE) {
    return true;
  } else if (cadState == RADIOLIB_LORA_DETECTED) {
    return false;
  } else {
    Serial.printf("[Nodo] Error CAD: %d\n", cadState);
    return false;
  }
}

// --------------------------------------------------------------------------------
// Lógica de estados (LoRa): handleBeacon, handleSchedule, etc.
// --------------------------------------------------------------------------------
void sendRequest() {
  Message reqMsg;
  reqMsg.header.type     = REQUEST;
  reqMsg.header.flag     = NODE_FLAG;
  reqMsg.header.payload  = 0;
  reqMsg.header.nodeTxId = thisNodeId;
  reqMsg.header.nodeRxId = GATEWAY;

  uint8_t buffer[sizeof(Message)];
  memcpy(buffer, &reqMsg, sizeof(Message));

  if (attemptChannelAccess(2, 5000)) {
    int st = radio.transmit(buffer, sizeof(Message));
    if (st == RADIOLIB_ERR_NONE) {
      Serial.println("[Nodo] Request enviado (RTX)!");
      currentNodeState = NODE_WAIT_SCHEDULE;
    } else {
      Serial.print("[Nodo] Error en Request TX: ");
      Serial.println(st);
    }
  }
}

void handleBeacon(const Message& msg) {
  Serial.println("[Nodo] Beacon recibido => Intentar canal...");
  if (attemptChannelAccess(2, 7000)) {
    sendRequest();
    currentNodeState = NODE_REQUEST_SENT;
  } else {
    Serial.println("[Nodo] Canal ocupado => quedo en WAIT_BEACON...");
    currentNodeState = NODE_WAIT_BEACON;
  }
}

void handleSchedule(const SchedulePacket& sch) {
  Serial.println("[Nodo] Recibido SCHEDULE (SRX).");

  bool found = false;
  for (uint8_t i = 0; i < sch.nodeCount; i++) {
    if (sch.nodeIds[i] == thisNodeId) {
      assignedSF   = sch.sfs[i];
      assignedSlot = sch.slots[i];
      found = true;
      break;
    }
  }
  if (found) {
    // Suponiendo: 10s + slot*5s => Espera hasta tu slot
    uint32_t slotDelayMs = 10000UL + (assignedSlot * 5000UL);

    // Guardamos en RTC para el deep_sleep
    savedState  = NODE_WAIT_SLOT;
    savedSF     = assignedSF;
    savedSlot   = assignedSlot;
    sleepTimeMs = slotDelayMs;

    Serial.printf("[Nodo] Programado SF=%u, slot=%u => duermo %u ms...\n", 
                   assignedSF, assignedSlot, slotDelayMs);
    goToDeepSleep(slotDelayMs);
  } else {
    // Si no estamos en el SCHEDULE, dormimos un rato y reintentamos
    savedState  = NODE_WAIT_BEACON;
    savedSF     = 12;
    savedSlot   = 0xFF;
    sleepTimeMs = 40000UL; // ejemplo
    Serial.printf("[Nodo] Nodo no en SCHEDULE => duermo %u ms\n", sleepTimeMs);
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
  Serial.println("[Nodo] Envío DATA...");

  Message dataMsg;
  dataMsg.header.type     = DATA;
  dataMsg.header.flag     = NODE_FLAG;
  dataMsg.header.payload  = 0;
  dataMsg.header.nodeTxId = thisNodeId;
  dataMsg.header.nodeRxId = GATEWAY;
  dataMsg.sf        = assignedSF;
  dataMsg.dataValue = 0b1111111; // Ejemplo, tu medición real

  uint8_t buffer[sizeof(Message)];
  memcpy(buffer, &dataMsg, sizeof(Message));

  if (attemptChannelAccess(1, 500)) {
    int st = radio.transmit(buffer, sizeof(Message));
    if (st == RADIOLIB_ERR_NONE) {
      Serial.println("[Nodo] DATA enviado OK (DTX)");
    } else {
      Serial.print("[Nodo] Error enviando DATA: ");
      Serial.println(st);
    }
  }

  // Dormimos 10s (por ejemplo) tras enviar
  goToDeepSleep(10000);
}

// --------------------------------------------------------------------------------
// Deep Sleep
// --------------------------------------------------------------------------------
void goToDeepSleep(uint32_t ms) {
  if (ms == 0) ms = 1000;
  esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);
  Serial.printf("[Nodo] => Deep Sleep %u ms\n", ms);
  esp_deep_sleep_start();
}

void resumeFromDeepSleep() {
  Serial.println("[Nodo] => Reanudando tras deep sleep");

  // Restauramos el estado guardado
  currentNodeState = savedState;
  assignedSF       = savedSF;
  assignedSlot     = savedSlot;

  if (currentNodeState == NODE_WAIT_SLOT) {
    // Directo a enviar data
    currentNodeState = NODE_SEND_DATA;
    radio.setSpreadingFactor(assignedSF);
    sendData();
    currentNodeState = NODE_SLEEP;
  } else {
    // Volvemos a esperar beacon
    startReceive();
  }
}
