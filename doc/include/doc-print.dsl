<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA dsssl>
]>

<style-sheet>
<style-specification use="docbook">
<style-specification-body> 

;;(define %paper-type%
;;  "USletter")

;;(define %page-height%
;;  11in)

;;(define %top-margin%
;;  0in)

;;(define %bottom-margin%
;;  0in)

(define %generate-article-toc%
  ;; Should a Table of Contents be produced for Articles?
  #t)

(define (toc-depth nd)
  ;; used to be 2
  4)

(define %generate-article-titlepage-on-separate-page%
  ;; Should the article title page be on a separate page?
  #t)

(define %section-autolabel%
  ;; Are sections enumerated?
  #t)

(define %footnote-ulinks%
  ;; Generate footnotes for ULinks?
  #f)

(define %bop-footnotes%
  ;; Make "bottom-of-page" footnotes?
  #f)

(define %body-start-indent%
  ;; Default indent of body text
  0pi)

(define %para-indent-firstpara%
  ;; First line start-indent for the first paragraph
  0pt)

(define %para-indent%
  ;; First line start-indent for paragraphs (other than the first)
  0pt)

(define %block-start-indent%
  ;; Extra start-indent for block-elements
  0pt)

(define formal-object-float
  ;; Do formal objects float?
  #t)

(define %hyphenation%
  ;; Allow automatic hyphenation?
  #t)

(define %admon-graphics%
  ;; Use graphics in admonitions?
  #f)

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="docbook.dsl">

</style-sheet>

