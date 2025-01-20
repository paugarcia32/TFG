#import "comparisonTableLoRaWAN.typ": comparisonTableLoRaWAN
#import "otherLoRaWANParamsTable.typ": otherLoRaWANParamsTable



#let beelanModules = (
  ("SX127x", "series LoRa modules (SX1273, SX1277, SX1278 and SX1279)"),
)

#let beelanModulesList = [
  #show heading: set heading()
  #show table.cell.where(y: 0): set text(weight: "bold")
  === Beelan Modules
  #pad()[]
  #figure(
table(
    columns: (20%, 80%),
    stroke: none,
    inset: 10pt,
    table.hline(y: 1),
    table.vline(x: 1),
    table.header([Module], [Description]),
    ..for (module, desc) in beelanModules {
      (module, desc)
    }
  ), caption: "Beelan Modules List",
  )

]

#let arduino_lorawanModules = (
  ("SX127x", "series LoRa modules (SX1276-based)"),
)

#let arduino_lorawanModulesList = [
  #show heading: set heading()
  #show table.cell.where(y: 0): set text(weight: "bold")
  === Arduino LoraWAN Modules
  #pad()[]
  #figure(
table(
    columns: (20%, 80%),
    stroke: none,
    inset: 10pt,
    table.hline(y: 1),
    table.vline(x: 1),
    table.header([Module], [Description]),
    ..for (module, desc) in arduino_lorawanModules {
      (module, desc)
    }
  ), caption: "Arduino LoRaWAN Modules List",
  )

]



#let comparisonContentLoRaWAN = [

  = Comparison Between Libraries LoRaWAN Libraries
  #pad()[]
  #par[
    For LoRaWAN, I've found many different  libraries, but a lot of them are only tested with very specific hardware, so on the following, you will find some of the most wide hardware tested libraries I've been able to find.
  ]

  == Beelan-LoRaWAN
  #pad()[]
  #par[
    In first place, there is the `Beelan LoRaWAN` @beelan_lorawan Library. This library is develop for be used in a generic platform, so is useful in our case.
  ]

  #beelanModulesList

  == Arduino LoRaWAN
  #pad()[]
  #par[
    In second place, there is the `MCCI Arduino LoRaWAN` @arduino_lorawan Library. The `arduino-lorawan` library provides a structured way of using the #link("https://github.com/mcci-catena/arduino-lmic")[arduino-lmic] library to send sensor data over The Things Network or a similar LoRaWAN-based data network.
  ]

  #arduino_lorawanModulesList



  #comparisonTableLoRaWAN

  #otherLoRaWANParamsTable
]