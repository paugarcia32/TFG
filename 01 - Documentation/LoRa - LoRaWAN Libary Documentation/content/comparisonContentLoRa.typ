#import "comparisonTable.typ": comparisonTable
#import "otherLoRaParamsTable.typ": otherLoRaParamsTable



#let radiolibModules = (
  ("CC1101", "FSK radio module"),
  ("LLCC68", "LoRa module"),
  ("LR11x0", "series LoRa/GFSK modules (LR1110, LR1120, LR1121)"),
  ("nRF24L01", "2.4 GHz module"),
  ("RF69", "FSK/OOK radio module"),
  ("RFM2x", "series FSK modules (RFM22, RFM23)"),
  ("RFM9x", "series LoRa modules (RFM95, RFM96, RFM97, RFM98)"),
  ("Si443x", "series FSK modules (Si4430, Si4431, Si4432)"),
  ("STM32WL", "integrated microcontroller/LoRa module"),
  ("SX126x", "series LoRa modules (SX1261, SX1262, SX1268)"),
  ("SX127x", "series LoRa modules (SX1272, SX1273, SX1276, SX1277, SX1278, SX1279)"),
  ("SX128x", "series LoRa/GFSK/BLE/FLRC modules (SX1280, SX1281, SX1282)"),
  ("SX123x", "FSK/OOK radio modules (SX1231, SX1233)")
)

#let radiolibModulesList = [
  #show heading: set heading()
  #show table.cell.where(y: 0): set text(weight: "bold")
  === RadioLib Modules
  #pad()[]
  #figure(
table(
    columns: (20%, 80%),
    stroke: none,
    inset: 10pt,
    table.hline(y: 1),
    table.vline(x: 1),
    table.header([Module], [Description]),
    ..for (module, desc) in radiolibModules {
      (module, desc)
    }
  ), caption: "RadioLib Modules List",
  )

]


#let loraMeshModules = (
  ("SX126x", "series LoRa modules (SX1262, SX1268)"),
  ("SX127x", "series LoRa modules (SX1276, SX1278)"),
  ("SX128x", "series LoRa/GFSK/BLE/FLRC modules (SX1280)"),
)

#let loraMeshbModulesList = [
  #show heading: set heading()
  #show table.cell.where(y: 0): set text(weight: "bold")
  === RadioLib Modules
  #pad()[]
  #figure(
table(
    columns: (20%, 80%),
    stroke: none,
    inset: 10pt,
    table.hline(y: 1),
    table.vline(x: 1),
    table.header([Module], [Description]),
    ..for (module, desc) in loraMeshModules {
      (module, desc)
    }
  ), caption: "Lora Mesher Modules List",
  )

]



#let inoModules = (
  ("SX127x", "series LoRa modules (SX1272, SX1273, SX1276, SX1277, SX1278, SX1279)"),
  ("Dragino", "Lora Shield"),
  ("HopeRF", "RFM95W, RFM96W, and RFM98W"),
  ("Modtronix", "inAir4, inAir9, and inAir9B")
)

#let inoModulesList = [
  #show heading: set heading()
  #show table.cell.where(y: 0): set text(weight: "bold")
  === Arduino LoRa Modules
  #pad()[]
    #figure(
      table(
      columns: (20%, 80%),
      stroke: none,
      inset: 10pt,
      table.hline(y: 1),
      table.vline(x: 1),
      table.header([Module], [Description]),
      ..for (module, desc) in inoModules {
        (module, desc)
      }
    ), caption: "Arduino LoRa Modules List",
  )

]

#let heltecModules = (
  ("SX1262", "Semtech SX1262 sub-GHz transceiver"),
)

#let heltecModulesList = [
  #show heading: set heading()
  #show table.cell.where(y: 0): set text(weight: "bold")
  === Unnoficial Heltec Modules
  #pad()[]
  #figure(
    table(
    columns: (20%, 80%),
    stroke: none,
    inset: 10pt,
    table.hline(y: 1),
    table.vline(x: 1),
    table.header([Module], [Description]),
    ..for (module, desc) in heltecModules {
      (module, desc)
    }
  ), caption: "Heltec Modules List",
  )

]


#let comparisonContentLoRa = [
  = Comparison Between Libraries LoRa Libraries
  #pad()[]
  #par[
    As of today, I have found only three different libraries that allow Arduino to use LoRa, either using embedded LoRa modules or external modules.
  ]

  == Unofficial Heltec <unofficial>
  #pad()[]
  #par[
    First, there is an `unofficial Heltec` library @heltec_esp32_lora_v3 that tries to implement all the functions related to Heltec Boards, because the official documentation is not intuitive.
  ]
  #par[
    This library, as the documentation says, is supposed to use the `SX-1262 LoRa module`, so I don't know if it would work if you try to use this library with any other module.
  ]
  #par[
    Also, in the documentation, we can find that this library reuses the #link("https://github.com/ropg/heltec_esp32_lora_v3?tab=readme-ov-file#radiolib")[`RadioLib`] @radiolib library. As we will see in @RadioLib, the `RadioLib library` allows the use of many more `SX LoRa modules`, so it could allow us to implement other `SX LoRa modules`.
  ]

  #heltecModulesList

  == Arduino LoRa <inoLora>
  #pad()[]

  #par[
    In second place, we have the `Arduino LoRa library` @arduino_lora. This library is also an unofficial library maintained by the comunity, which allows the `Semtech SX1276/77/78/79` based boards.
  ]

  #inoModulesList

  == RadioLib <RadioLib>
  #pad()[]

  #par[
    In third place, we have the `RadioLib library` @radiolib. `RadioLib` allows its users to integrate all sorts of different wireless communication modules & protocols into a single system.
  ]
    #par[
    RadioLib natively supports Arduino, but can run in non-Arduino environments as well.
  ]

  #radiolibModulesList
  #pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]#pad()[]

  #comparisonTable
  #pad()[]#pad()[]

  #otherLoRaParamsTable
]
