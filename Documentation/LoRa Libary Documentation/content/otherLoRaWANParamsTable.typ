#let otherLoRaWANParamsTable = [

  #show table.cell.where(y: 0): set text(weight: "bold")
  #set table(align: (x, _) => if x == 0 { left } else { center })

  == Extra parameters to take into account
  #pad()[]
  #par()[
    For this table, is has been taken into account the following:
    - The library is considered updated if there has been some commit into the original repository in the last 12 months.
    - The documentation is considered as good, if there are examples of code of how to use the API of the library and it helps to use the library.
  ]

  #figure(
    table(
      stroke: none,
      inset: 10pt,
      columns: 3,
      table.hline(y: 1),
      table.vline(x: 1),

      table.header([],[Beelan LoRaWAN],[Arduino LoRaWAN]),
      [Library Updated], [☒], [☒],
      [Good Documentation], [☒], [☒],
    ),
    caption: "Extra Parameters Table",
  ) <otherLoRaWANParamsTable>


]
