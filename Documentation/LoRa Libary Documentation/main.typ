#import "conf.typ": conf
#import "content/intro.typ": intro
#import "content/comparisonContentLoRa.typ": comparisonContentLoRa
#import "content/comparisonContentLoRaWAN.typ": comparisonContentLoRaWAN
#import "acronyms.typ": acronymList

#show: doc => conf(
  title: [
    Libraries Documentation
  ],
  authors: (
    (
      name: "Pau Garcia Calderó",
      affiliation: "EETAC - UPC",
      email: "paugarcia32@gmail.com",
    ),
  ),
  logo: "assets/logos/eetac.png",
  doc,
)

#intro
#comparisonContentLoRa
#comparisonContentLoRaWAN
