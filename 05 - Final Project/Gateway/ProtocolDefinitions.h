#ifndef PROTOCOL_DEFINITIONS_H
#define PROTOCOL_DEFINITIONS_H

#include <Arduino.h>

// ----------------------------------------------------------------------------------
// Para empaquetar las estructuras sin bytes de relleno (padding).
// (En algunos compiladores te aseguras de que sizeof(...) coincida con los campos exactos)
// ----------------------------------------------------------------------------------
#pragma pack(push, 1)

// ----------------------------------------------------------------------------------
// Enums para tipos de mensaje (2 bits)
// ----------------------------------------------------------------------------------
enum MessageType : uint8_t {
  BEACON   = 0b00,
  REQUEST  = 0b01,
  SCHEDULE = 0b10,
  DATA     = 0b11
};

// ----------------------------------------------------------------------------------
// Flag que indica si el transmisor es nodo o gateway (1 bit)
// ----------------------------------------------------------------------------------
enum HeaderFlag : uint8_t {
  NODE_FLAG     = 0b0,
  GATEWAY_FLAG  = 0b1
};

// ----------------------------------------------------------------------------------
// Ejemplo adicional, por si se usan distintos tipos de Data en el futuro (no obligado).
// ----------------------------------------------------------------------------------
enum DataMessageType : uint8_t {
  DATA_TEMPERATURE = 0b00,
  DATA_PRESSURE    = 0b01
};

// ----------------------------------------------------------------------------------
// Enums para IDs de Nodos (8 bits)
// ----------------------------------------------------------------------------------
enum NodeID : uint8_t {
  GATEWAY   = 0x00,
  NODE_0    = 0x01,
  NODE_1    = 0x02,
  NODE_2    = 0x03,
  NODE_3    = 0x04,
  BROADCAST = 0xFF
};

// ----------------------------------------------------------------------------------
// Estructura de la cabecera (3 bytes en total).
// Nota: Cada campo es 1 byte, pero en tu lógica los 2 bits + 1 bit + 5 bits
// van en 'type' y 'flag' de forma conceptual. Realmente ocupan 1 byte entero
// cada uno en este ejemplo.
// ----------------------------------------------------------------------------------
struct Header {
  // byte 1
  MessageType type;    // (2 bits usados)  En la práctica, ocupa 1 byte
  HeaderFlag  flag;    // (1 bit usado)    En la práctica, ocupa 1 byte
  uint8_t     payload; // 5 bits libres   En la práctica, ocupa 1 byte

  // byte 2
  NodeID nodeTxId;     // 8 bits de ID del transmisor

  // byte 3
  NodeID nodeRxId;     // 8 bits de ID del receptor
};

// ----------------------------------------------------------------------------------
// Estructura de mensaje genérico (para BEACON, REQUEST, etc.)
//   - Tamaño total: 3 bytes de header + 1 byte sf + 2 bytes dataValue = 6 bytes
// ----------------------------------------------------------------------------------
struct Message {
  Header  header;      // 3 bytes
  uint8_t sf;          // 1 byte
  uint16_t dataValue;  // 2 bytes
};

// ----------------------------------------------------------------------------------
// Estructura para guardar información de cada nodo en el Gateway
// (guardamos nodeId y su SF calculado)
// ----------------------------------------------------------------------------------
struct NodeInfo {
  NodeID  nodeId;
  uint8_t optimalSF;
};

// ----------------------------------------------------------------------------------
// Estructura especial para SCHEDULE
//  - 3 bytes de Header
//  - 1 byte  nodeCount
//  - 4 bytes nodeIds[MAX_NODES]
//  - 4 bytes sfs[MAX_NODES]
//  - 4 bytes slots[MAX_NODES]
// => total = 3 + 1 + 4 + 4 + 4 = 16 bytes (para MAX_NODES=4)
// ----------------------------------------------------------------------------------
#define MAX_NODES 4

struct SchedulePacket {
  Header  header;           // 3 bytes
  uint8_t nodeCount;        // 1 byte

  NodeID  nodeIds[MAX_NODES];  // 4 bytes
  uint8_t sfs[MAX_NODES];      // 4 bytes
  uint8_t slots[MAX_NODES];    // 4 bytes
};

#pragma pack(pop)

// ----------------------------------------------------------------------------------
// Constantes varias
// ----------------------------------------------------------------------------------
#define BEACON_INTERVAL   40  // segundos entre beacons
#define REQUEST_WINDOW_MS 10000UL  // 10 segundos para recibir REQUESTs
#define MAX_SLOTS         MAX_NODES

// ----------------------------------------------------------------------------------
// Arreglo global (SOLO para Gateway) para gestionar nodos
// ----------------------------------------------------------------------------------
static NodeInfo nodes[MAX_NODES];
static uint8_t  nodeCount = 0;

// ----------------------------------------------------------------------------------
// Variables para el manejo del estado de RadioLib (opcional)
// ----------------------------------------------------------------------------------
volatile bool operationDone  = false;
int          operationState  = 0;

#endif // PROTOCOL_DEFINITIONS_H
