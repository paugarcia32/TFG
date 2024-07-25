#import "comparisonTable.typ": comparisonTable

#let comparisonContent = [
  = Comparison Between Libraries

  #par[
    As of today, I have found only three different libraries that allow Arduino to use LoRa, either using embedded LoRa modules or external modules.
  ]

  == Unofficial Heltec <unofficial>
  #par[
    First, there is an unofficial Heltec library @heltec_esp32_lora_v3 that tries to implement all the functions related to Heltec Boards, because the official documentation is not intuitive.
  ]
  #par[
    This library, as the documentation says, is supposed to use the SX-1262 LoRa module, so I don't know if it would work if you try to use this library with any other module.
  ]
  #par[
    Also, in the documentation, we can find that this library reuses the #link("https://github.com/ropg/heltec_esp32_lora_v3?tab=readme-ov-file#radiolib")[RadioLib] @radiolib library. As we will see in @RadioLib, the RadioLib library allows the use of many more SX LoRa modules, so it could allow us to implement other SX LoRa modules.
  ]

  == Arduino LoRa <inoLora>
  #par[
    In second place, we have the Arduino LoRa library @arduino_lora. This library is also an unofficial library maintained by the comunity, which allows the Semtech SX1276/77/78/79 based boards.
  ]

  == RadioLib <RadioLib>
  #par[
    In last place, we have the RadioLib library @radiolib. RadioLib allows its users to integrate all sorts of different wireless communication modules & protocols into a single system.
  ]

  #comparisonTable
]
