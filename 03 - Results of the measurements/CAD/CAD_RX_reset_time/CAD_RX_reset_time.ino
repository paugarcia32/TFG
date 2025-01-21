

#include <RadioLib.h>
#include <Arduino.h>

#define NSS_PIN 8
#define DIO1_PIN 14
#define NRST_PIN 12
#define BUSY_PIN 13

SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);

bool receiving = false;
volatile bool cadFlag = false;
unsigned long receivedTimestamp = 0;
unsigned long cadOperationalTimestamp = 0;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
  cadFlag = true;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  int state = radio.begin(868.0, 125.0, 7, 5);
  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.print("[RX] Error al inicializar la radio. Código: ");
    Serial.println(state);
    while (true)
      ;
  }

  radio.setDio1Action(setFlag);
  radio.startChannelScan();
}

void loop()
{
  if (cadFlag)
  {
    cadFlag = false;

    int cadResult = radio.getChannelScanResult();

    if (receiving)
    {
      receiving = false;

      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE)
      {
        receivedTimestamp = millis();
        Serial.print("[RX] Paquete recibido -> ");
        Serial.println(str);
        Serial.print("[TIMESTAMP] Recibido en: ");
        Serial.println(receivedTimestamp);
      }
      else
      {
        Serial.print("[RX] Error al leer paquete. Código: ");
        Serial.println(state);
      }

      radio.startChannelScan();
    }
    else
    {
      if (cadResult == RADIOLIB_LORA_DETECTED)
      {
        int state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE)
        {
          receiving = true;
        }
        else
        {
          Serial.print("[RX] Error al iniciar recepción. Código: ");
          Serial.println(state);
          radio.startChannelScan();
        }
      }
      else if (cadResult == RADIOLIB_CHANNEL_FREE)
      {
        cadOperationalTimestamp = millis();
        Serial.print("[TIMESTAMP] CAD operativo en: ");
        Serial.println(cadOperationalTimestamp);
        radio.startChannelScan();
      }
      else
      {
        radio.startChannelScan();
      }
    }
  }
}
