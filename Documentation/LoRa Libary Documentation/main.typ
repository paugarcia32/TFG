#import "conf.typ": conf
#import "content/intro.typ": intro
#import "content/comparisonContent.typ": comparisonContent

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
#comparisonContent






