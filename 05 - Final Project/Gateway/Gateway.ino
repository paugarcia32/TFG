#include <Arduino.h>
#include <RadioLib.h>
#include "ProtocolDefinitions.h"


#include <ArduinoBLE.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>  


#if defined(ESP32)
#define LORA_CS 8
#define LORA_DIO0 14
#define LORA_RST 12
#define LORA_DIO1 13
float frequency = 868.0;
#endif


SX1262 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);

static NodeID gatewayNodeId = GATEWAY;
static uint16_t beaconCounter = 0;

struct ReceivedData {
  NodeID nodeId;
  uint16_t dataValue;
  uint8_t sfUsed;
};

static std::vector<ReceivedData> dataList;

enum GatewayState {
  GW_IDLE,
  GW_REQUEST_WINDOW,
  GW_SEND_SCHEDULE,
  GW_WAIT_DATA
};

GatewayState currentState = GW_IDLE;

unsigned long lastBeaconTime = 0;
const unsigned long BEACON_INTERVAL_MS = BEACON_INTERVAL * 1000UL;  
unsigned long requestWindowStart = 0;

volatile bool newPacketAvailable = false;


Preferences preferences;
BLEService wifiConfigService("0x008D"); 
BLEStringCharacteristic wifiCredentialsCharacteristic(
  "c6e0bf23-f27d-4e1a-9ed8-fdfbd1b81e33",
  BLERead | BLEWrite | BLEEncryption,
  128
);


bool wifiConnected = false; 


void gatewayInitRadio();
void setFlag();
void handleReceivedPacket();
uint8_t calculateOptimalSF(float rssi, float snr);

void sendBeacon();
void sendSchedule();
void startReceive();

void waitForDataFromNodes();

void sendDataToServer(const std::vector<ReceivedData>& dataVec);


void onDataReceived(BLEDevice central, BLECharacteristic characteristic) {
  if (characteristic.uuid() == wifiCredentialsCharacteristic.uuid()) {
    int length = characteristic.valueLength();
    const uint8_t* data = characteristic.value();

    char buffer[length + 1];
    memcpy(buffer, data, length);
    buffer[length] = '\0';

    String receivedData = String(buffer);

    Serial.print("Received BLE data: ");
    Serial.println(receivedData);

    
    int separatorIndex = receivedData.indexOf(':');
    if (separatorIndex > 0) {
      String ssid = receivedData.substring(0, separatorIndex);
      String password = receivedData.substring(separatorIndex + 1);

      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(password);

      
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);


      WiFi.begin(ssid.c_str(), password.c_str());
      Serial.println("Connecting to Wi-Fi...");

      int maxTries = 20;
      while (WiFi.status() != WL_CONNECTED && maxTries > 0) {
        delay(500);
        Serial.print(".");
        maxTries--;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
      } else {
        Serial.println("\nFailed to connect to Wi-Fi.");
        wifiConnected = false;
      }
    } else {
      Serial.println("Invalid data format. Expected 'SSID:Password'.");
    }
  }
}

void initBLE() {
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  Serial.println("BLE initialized");

  BLE.setDeviceName("ESP32 Wi-Fi Config");
  BLE.setLocalName("ESP32 Wi-Fi Config");

  BLE.setAdvertisedService(wifiConfigService);

  wifiConfigService.addCharacteristic(wifiCredentialsCharacteristic);
  BLE.addService(wifiConfigService);

  wifiCredentialsCharacteristic.setEventHandler(BLEWritten, onDataReceived);
  BLE.advertise();
  Serial.println("Advertising BLE service...");
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== [GATEWAY] Iniciando ===");

  preferences.begin("wifiCreds", false);


  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");

  if (ssid != "") {
    Serial.println("Found stored Wi-Fi credentials.");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Attempting to connect to Wi-Fi...");

    int maxTries = 20;
    while (WiFi.status() != WL_CONNECTED && maxTries > 0) {
      delay(500);
      Serial.print(".");
      maxTries--;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      wifiConnected = true;
    } else {
      Serial.println("\nFailed to connect to Wi-Fi.");
      wifiConnected = false;
    }
  } else {
    Serial.println("No stored Wi-Fi credentials in NVS.");
    wifiConnected = false;
  }

  
  initBLE();

  
  gatewayInitRadio();

  Serial.println("=== [GATEWAY] Setup Completo ===");
}


void loop() {
  
  BLEDevice central = BLE.central();
  if (central) {
    while (central.connected()) {
      BLE.poll();
      delay(10);
    }
  }

  
  unsigned long now = millis();

  if (newPacketAvailable) {
    newPacketAvailable = false;
    handleReceivedPacket();
  }

  switch (currentState) {
    case GW_IDLE:
      if ((now - lastBeaconTime) >= BEACON_INTERVAL_MS) {
        sendBeacon();
        lastBeaconTime = now;

        currentState = GW_REQUEST_WINDOW;
        requestWindowStart = now;
        startReceive();
      }
      
      break;

    case GW_REQUEST_WINDOW:
      if ((now - requestWindowStart) >= REQUEST_WINDOW_MS) {
        currentState = GW_SEND_SCHEDULE;
      }
      break;

    case GW_SEND_SCHEDULE:
      sendSchedule();
      currentState = GW_WAIT_DATA;
      break;

    case GW_WAIT_DATA:
      waitForDataFromNodes();
      nodeCount = 0;
      currentState = GW_IDLE;
      break;
  }

  delay(10);
}


void gatewayInitRadio() {
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[Gateway] Radio inicializada correctamente!");
  } else {
    Serial.print("[Gateway] Error iniciando la radio. Codigo: ");
    Serial.println(state);
    while (true) {
      delay(1000);
    }
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
  newPacketAvailable = true;
}

void handleReceivedPacket() {
  int packetSize = radio.getPacketLength();
  if (packetSize <= 0) {
    return;
  }

  uint8_t buffer[64];
  int state = radio.readData(buffer, packetSize);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[Gateway] Error leyendo paquete. Code=");
    Serial.println(state);
    startReceive();
    return;
  }

  if (packetSize == sizeof(Message)) {
    Message msg;
    memcpy(&msg, buffer, sizeof(Message));

    switch (msg.header.type) {
      case REQUEST:
        if (msg.header.flag == NODE_FLAG) {
          float rssi = radio.getRSSI();
          float snr = radio.getSNR();
          uint8_t sf = calculateOptimalSF(rssi, snr);

          Serial.println("RRX");
          Serial.print("[Gateway] REQUEST de nodo=0x");
          Serial.print(msg.header.nodeTxId, HEX);
          Serial.print(" => RSSI=");
          Serial.print(rssi);
          Serial.print(" dBm, SNR=");
          Serial.print(snr);
          Serial.print(" => SF=");
          Serial.println(sf);

          bool found = false;
          for (uint8_t i = 0; i < nodeCount; i++) {
            if (nodes[i].nodeId == msg.header.nodeTxId) {
              nodes[i].optimalSF = sf;
              found = true;
              Serial.println(" [Gateway] Nodo ya estaba registrado. SF actualizado.");
              break;
            }
          }
          if (!found && nodeCount < MAX_NODES) {
            nodes[nodeCount].nodeId = msg.header.nodeTxId;
            nodes[nodeCount].optimalSF = sf;
            Serial.print(" [Gateway] Nuevo nodo idx=");
            Serial.println(nodeCount);
            nodeCount++;
          }
        } else {
          Serial.println("[Gateway] REQUEST pero no viene de un NODO_FLAG?");
        }
        break;

      case DATA:
        if (msg.header.flag == NODE_FLAG) {
          Serial.println("DRX");
          float temperature = (float)msg.dataValue / 10.0f;
          Serial.print("[Gateway] DATA de nodo 0x");
          Serial.print(msg.header.nodeTxId, HEX);
          Serial.print(": valor=");
          Serial.print(msg.dataValue);
          Serial.print(" => Temp=");
          Serial.print(temperature);
          Serial.print("ºC, SF=");
          Serial.println(msg.sf);

          ReceivedData rd;
          rd.nodeId = msg.header.nodeTxId;
          rd.dataValue = msg.dataValue;
          rd.sfUsed = msg.sf;
          dataList.push_back(rd);

          Serial.print("[Gateway] DATA de nodo 0x");
          Serial.print(msg.header.nodeTxId, HEX);
          Serial.print(": dataValue=");
          Serial.print(msg.dataValue, BIN);
          Serial.print(" => temp=");
          Serial.print((float)msg.dataValue);
          Serial.print(" C, SF=");
          Serial.println(msg.sf);
        } else {
          Serial.println("[Gateway] DATA pero no viene de un NODO_FLAG?");
        }
        break;

      default:
        Serial.print("[Gateway] Recibido un Mensaje type=");
        Serial.print(msg.header.type);
        Serial.println(" no esperado aquí.");
        break;
    }
  } else if (packetSize == sizeof(SchedulePacket)) {
    Serial.println("[Gateway] ¡Schedule recibido en Gateway! (inusual).");
  } else {
    Serial.print("[Gateway] Paquete con tamano inesperado=");
    Serial.println(packetSize);
  }

  startReceive();
}

uint8_t calculateOptimalSF(float rssi, float snr) {
  const int sensitivitySF[] = { -125, -127, -130, -132, -135, -137 };
  const float snrLimit[] = { -7.5, -10.0, -12.5, -15.0, -17.5, -20.0 };

  for (int i = 0; i < 6; i++) {
    if (rssi >= sensitivitySF[i] && snr >= snrLimit[i]) {
      return 7 + i; 
    }
  }
  return 12;
}

void sendBeacon() {
  Serial.println("[Gateway] Enviando Beacon en SF12...");

  radio.setSpreadingFactor(12);

  Message beaconMsg;
  beaconMsg.header.type = BEACON;
  beaconMsg.header.flag = GATEWAY_FLAG;
  beaconMsg.header.payload = 0;
  beaconMsg.header.nodeTxId = gatewayNodeId;
  beaconMsg.header.nodeRxId = BROADCAST;

  uint8_t buffer[sizeof(Message)];
  memcpy(buffer, &beaconMsg, sizeof(Message));

  int state = radio.transmit(buffer, sizeof(Message));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("[Gateway] Beacon enviado. ");
    Serial.println("BTX");
  } else {
    Serial.print("[Gateway] Error enviando Beacon. Code=");
    Serial.println(state);
  }
}

void sendSchedule() {
  Serial.println("[Gateway] Enviando SCHEDULE...");

  radio.setSpreadingFactor(12);

  SchedulePacket sch;
  sch.header.type = SCHEDULE;
  sch.header.flag = GATEWAY_FLAG;
  sch.header.payload = 0;
  sch.header.nodeTxId = gatewayNodeId;
  sch.header.nodeRxId = BROADCAST;

  sch.nodeCount = nodeCount;

  for (uint8_t i = 0; i < nodeCount; i++) {
    sch.nodeIds[i] = nodes[i].nodeId;
    sch.sfs[i] = nodes[i].optimalSF;
    sch.slots[i] = i;
  }

  uint8_t buffer[sizeof(SchedulePacket)];
  memcpy(buffer, &sch, sizeof(SchedulePacket));

  int state = radio.transmit(buffer, sizeof(SchedulePacket));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[Gateway] SCHEDULE enviado OK!");
    Serial.println("STX");
  } else {
    Serial.print("[Gateway] Error enviando SCHEDULE. Code=");
    Serial.println(state);
  }
}

void startReceive() {
  int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("[Gateway] Error al poner RX. Code=");
    Serial.println(state);
  }
}


void waitForDataFromNodes() {
  Serial.println("[Gateway] *** Enviando SCHEDULE completado ***");
  Serial.println("[Gateway] Esperamos 10s antes de empezar slots de Data...");
  delay(10000);

  Serial.println("[Gateway] Iniciando recepción de DATA en slots de 5s.");

  for (uint8_t i = 0; i < MAX_SLOTS; i++) {
    Serial.print("[Gateway] Slot #");
    Serial.print(i);
    Serial.println(" (5s de recepción)");

    if (i < nodeCount) {
      uint8_t sfNode = nodes[i].optimalSF;
      radio.setSpreadingFactor(sfNode);
      Serial.print("[Gateway] SF=");
      Serial.println(sfNode);
    } else {
      radio.setSpreadingFactor(12);
      Serial.print("[Gateway] Slot ");
      Serial.print(i);
      Serial.println(" sin nodo asignado, SF=12.");
    }

    startReceive();

    unsigned long start = millis();
    while (millis() - start < 5000UL) {
      if (newPacketAvailable) {
        newPacketAvailable = false;
        handleReceivedPacket();
      }
      delay(50);
    }
  }

  Serial.println("[Gateway] Terminó la recepción de Data de todos los slots.");
  Serial.println("[Gateway] Esperamos 10s para (simular) enviar datos por WiFi/serial...");
  delay(10000);

  Serial.println("[Gateway] RESUMEN DE DATOS RECIBIDOS:");
  for (const auto& d : dataList) {
    float temp = d.dataValue / 10.0;
    Serial.print(" - Nodo=0x");
    Serial.print(d.nodeId, HEX);
    Serial.print(", dataValue=");
    Serial.print(d.dataValue);
    Serial.print(" => ");
    Serial.print(temp);
    Serial.print(" C, SF=");
    Serial.println(d.sfUsed);
  }

  
  if (wifiConnected && dataList.size() > 0) {
    sendDataToServer(dataList);
  }

  
  radio.setSpreadingFactor(12);
  Serial.println("[Gateway] SF inicial restaurado a 12.");
  dataList.clear();
  nodeCount = 0;
  currentState = GW_IDLE;
}


void sendDataToServer(const std::vector<ReceivedData>& dataVec) {
  HTTPClient http;
  

  const char* serverUrl = "http://192.168.1.40:3000/data";

  Serial.println("[Gateway] Enviando JSON al servidor...");

 
  String json = "[";
  for (size_t i = 0; i < dataVec.size(); i++) {
    if (i > 0) {
      json += ",";
    }
    float temperature = dataVec[i].dataValue / 10.0;
    String nodeHex = String(dataVec[i].nodeId, HEX); 

    json += "{";
    json += "\"nodeId\":\"0x" + nodeHex + "\",";
    json += "\"dataValue\":" + String(dataVec[i].dataValue) + ",";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"sfUsed\":" + String(dataVec[i].sfUsed);
    json += "}";
  }
  json += "]";

  Serial.print("[Gateway] JSON a enviar: ");
  Serial.println(json);

  http.begin(serverUrl);                 
  http.addHeader("Content-Type","application/json");  

  int httpResponseCode = http.POST(json); 
  if (httpResponseCode > 0) {
    Serial.printf("[Gateway] POST code: %d\n", httpResponseCode);
    if (httpResponseCode == HTTP_CODE_OK || httpResponseCode == HTTP_CODE_CREATED) {
      
      String payload = http.getString();
      Serial.println("[Gateway] Respuesta del servidor: " + payload);
    }
  } else {
    Serial.printf("[Gateway] POST falló, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end(); 
}
