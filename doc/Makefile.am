## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

doc_DATA = programmers-guide.pdf programmers-guide/*

EXTRA_DIST = $(doc_DATA)

all:
	sgmltools -b ps -s include/doc-print.dsl programmers-guide.sgml
	ps2pdf programmers-guide.ps
	sgmltools -b html -s include/doc-html.dsl programmers-guide.sgml
