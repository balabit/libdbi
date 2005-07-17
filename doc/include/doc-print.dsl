<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA dsssl>
]>

<style-sheet>
<style-specification use="docbook">
<style-specification-body> 

;;(define %paper-type%
;;  "USletter")

(define %page-height%
  11in)

;;(define %default-quadding%
;;  'justify)

(define %top-margin%
  7pi)

(define %bottom-margin%
  10pi)

(define %footer-margin%
  4pi)

(define %header-margin%
  3pi)

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

(define %shade-verbatim%
  #t)

;;(define %indent-programlisting-lines%
;;  "    ")

;;(define %text-width% (- %page-width% (+ %left-margin% %right-margin%)))

;;Define the body width. (Change the elements in the formula rather
;;than the formula itself)
;;(define %body-width% (- %text-width% %body-start-indent%))

;; print varlistentry terms boldface so they stand out
(element (varlistentry term)
    (make paragraph
	  space-before: (if (first-sibling?)
			    %block-sep%
			    0pt)
	  keep-with-next?: #t
	  first-line-start-indent: 0pt
	  start-indent: (inherited-start-indent)
	  font-weight: 'bold
	  (process-children)))

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="docbook.dsl">

</style-sheet>

