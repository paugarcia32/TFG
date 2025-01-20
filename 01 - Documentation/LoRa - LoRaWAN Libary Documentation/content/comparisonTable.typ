#let comparisonTable = [

  #show table.cell.where(y: 0): set text(weight: "bold")
  #set table(align: (x, _) => if x == 0 { left } else { center })

  == Comparison table
  #figure(
    table(
      stroke: none,
      inset: 9pt,
      columns: 4,
      table.hline(y: 1),
      table.vline(x: 1),

      table.header([],[Unnoficial Heltec],[Arduino LoRa],[RadioLib]),
      [Frequency (MHz)], [☒], [☒], [☒],
      [Spreading Factor], [☒], [☒], [☒],
      [Signal Bandwidth (KHz)], [☒], [☒], [☒],
      [Coding Rate], [], [☒], [☒],
      [Preamble Length (bits)], [], [☒], [☒],
      [$P_(text("tx"))$ (dBm)], [☒], [☒], [☒],
      [Bus Type], [SPI], [SPI], [SPI],
      [Callbacks (Channel Activity Detection)], [☒], [☒], [],
      [Implicit / Explicit Header], [], [☒], [☒],
      [Write data to the packet], [☒], [☒], [☒],
      [RSSI], [], [☒], [☒],
      [SNR], [], [☒], [],
      [Packet Frequency Error], [], [☒], [],
      [Radio modes], [], [Idle \ Sleep], [],
      [Sync word], [], [☒], [☒],
      [CRC], [], [☒], [☒],
      [Invert IQ signals], [], [☒], [],
      [LNA gain], [], [☒], [],
    ),
    caption: "Library Table Comparison",
  ) <comparisonTable>

]












