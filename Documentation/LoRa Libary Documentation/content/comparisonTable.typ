#let comparisonTable = [

#show table.cell.where(y: 0): set text(weight: "bold")

#figure(
  table(
    align: center,
    stroke: none,
    inset: 10pt,
    columns: 4,
    table.header(
      [Library],
      [Freqiency (Hz)],
      [Spreading Factor],
      [$P_(text("tx"))$ (W)]
    ),
    table.hline(start: 0),
    [*Unnoficial Heltec* @heltec_esp32_lora_v3], [☒], [☐], [],
    [*Arduino LoRa* @arduino_lora],  [16.6], [104], [],
    [*RadioLib* @radiolib], [], [24.7], [],
  ), caption: "Library Table Comparison",

) <comparisonTable>
]








