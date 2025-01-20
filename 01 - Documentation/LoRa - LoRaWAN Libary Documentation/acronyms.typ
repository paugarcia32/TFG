#let acronyms = (
  ("LoRa", "Long Range"),
  ("MHz", "Mega Hertz"),
  ("KHz", "Kilo Hertz"),
  ("dBm", "decibel-milliwatts"),
  ("SNR", "signal-to-noise ratio"),
  ("RSSI", "Received Signal Strength Indicator"),
  ("CRC", "Cyclic Redundancy Check"),
  ("IQ", "In-Phase and Quadrature"),
  ("LNA", "Low-Noise Amplifier")
)


#let acronymList = [
  #show heading: set heading(numbering: none)
  #show table.cell.where(y: 0): set text(weight: "bold")
  = Acronyms
  #table(
  columns: (20%, 80%),
  stroke: none,
  inset: 10pt,
  table.hline(y: 1),
  table.vline(x: 1),
  table.header([*Acronym*],[*Meaning*]),
  ..for (acronym, meaning) in acronyms {
    (acronym, meaning)
  }
)

]


