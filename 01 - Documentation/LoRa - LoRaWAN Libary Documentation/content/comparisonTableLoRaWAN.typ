#let comparisonTableLoRaWAN = [

  #show table.cell.where(y: 0): set text(weight: "bold")
  #set table(align: (x, _) => if x == 0 { left } else { center })

  == Comparison table between LoRaWAN libraries
  #figure(
    table(
      stroke: none,
      inset: 9pt,
      columns: 3,
      table.hline(y: 1),
      table.vline(x: 1),

      table.header([],[Beelan],[Arduino LoRaWAN]),
      [Authentication Keys], [☒], [☒],
      [OTAA Activation], [☒], [☒],
      [LoRa Modules Sleep Mode], [☒], [],
      [LoRaWAN Class], [A & C], [Only A],

    ),
    caption: "LoRaWAN Library Table Comparison",
  ) <comparisonTable>

]