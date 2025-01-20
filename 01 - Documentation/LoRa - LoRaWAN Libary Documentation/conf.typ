#import "acronyms.typ": acronymList
#import "@preview/acrotastic:0.1.0": *
#let conf(
  title: none,
  authors: (),
  logo: none,
  doc,
) = {

  // Convertir la lista de autores en un solo string separado por comas.
  let authors_list = authors.map(author => author.name).join(", ")

  // Set the document's basic properties.
  set document(author: authors_list, title: title)
  set page(numbering: "1", number-align: right)

  // Save heading and body font families in variables.
  let body-font = "Open Sans"
  let sans-font = "Open Sans"

  // Set body font family.
  set text(font: body-font, lang: "en")
  show heading: set text(font: sans-font)

  set text(font: sans-font)

  set heading(numbering: "1.")

set page(
    header: context {
      set text(fill: gray);
      if counter(page).get().first() > 1 [
        #authors_list
        #h(1fr)
        UPC
      ];
    },
        footer: context {
      if counter(page).get().first() > 1 [
        #h(1fr)
        #counter(page).display()
      ];
    },

  );

  set par(linebreaks: "optimized", leading: 0.65em)
  // ------------------------------------------------------ //

  // Title page.
  // The page can contain a logo if you pass one with `logo: "logo.png"`.
  v(0.6fr)
  if logo != none {
    align(right, image(logo, width: 60%))
  }
  v(9.6fr)

  text(font: sans-font, 2em, weight: 700, title)

  // Author information.
  pad(
    top: 0.7em,
    right: 20%,
    grid(
      columns: (1fr,) * calc.min(3, authors.len()),
      gutter: 1em,
      ..authors.map(author => align(start, [
        #author.name \
        #author.affiliation \
        #link("mailto:" + author.email, author.email)
      ])),
    ),
  )

  v(2.4fr)
  pagebreak()

  // ------------------------------------------------------ //

  // Table of contents.
  outline(title: "Table of Contents", target: heading, depth: 3, indent: 2em)

  pagebreak()
  // ------------------------------------------------------ //

  set align(left)
  columns(1, doc)

  pagebreak()
  bibliography("sources.bib")

  pagebreak()
  acronymList
}