## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)
progguidedir = $(docdir)/programmers-guide
driverguidedir = $(docdir)/driver-guide

doc_DATA = programmers-guide.pdf  driver-guide.pdf
progguide_DATA = programmers-guide/*
driverguide_DATA = driver-guide/*

programmers_sources = programmers-guide.sgml libdbi-reference.sgml example-program.sgml libdbi-quickstart.sgml

driver_sources = driver-guide.sgml 

EXTRA_DIST = $(doc_DATA)  $(progguide_DATA) $(driverguide_DATA) $(programmers_sources) $(driver_sources)

all: programmers-guide.pdf programmers-guide/* driver-guide.pdf driver-guide/*

# jadetex spews out tons of useless crap. shut it up:
programmers-guide.pdf: $(programmers_sources)
#	@echo "Auto-generating Programmer's Guide reference SGML...";
#	./include/markup_to_docbook.pl libdbi-reference.tmpl api-documentation.src
#	@echo "Converting programmers-guide.sgml to PostScript...";
#	sgmltools -b ps -s include/doc-print.dsl programmers-guide.sgml > /dev/null 2>&1
#	@echo "Converting programmers-guide.ps to PDF...";
#	ps2pdf programmers-guide.ps
	@echo "Converting programmers-guide.sgml to PDF...";
	openjade -t tex -V tex-backend -d include/doc-print.dsl programmers-guide.sgml
# run jadetex three times to get the references right
	pdftex "&pdfjadetex" programmers-guide.tex
	pdftex "&pdfjadetex" programmers-guide.tex
	pdftex "&pdfjadetex" programmers-guide.tex

programmers-guide/*: $(programmers_sources)
	@echo "Converting programmers-guide.sgml to HTML...";
#	sgmltools -b html -s include/doc-html.dsl programmers-guide.sgml
	mkdir -p programmers-guide && cd programmers-guide && openjade -t sgml -d ../include/doc-html.dsl ../programmers-guide.sgml

driver-guide.pdf: $(driver_sources)
#	@echo "Converting driver-guide.sgml to PostScript...";
#	sgmltools -b ps -s include/doc-print.dsl driver-guide.sgml > /dev/null 2>&1
#	@echo "Converting driver-guide.ps to PDF...";
#	ps2pdf driver-guide.ps
	@echo "Converting driver-guide.sgml to PDF...";
	openjade -t tex -d include/doc-print.dsl driver-guide.sgml
# run jadetex three times to get the references right
	pdftex "&pdfjadetex" driver-guide.tex
	pdftex "&pdfjadetex" driver-guide.tex
	pdftex "&pdfjadetex" driver-guide.tex

driver-guide/*: $(driver_sources)
	@echo "Converting driver-guide.sgml to HTML...";
#	sgmltools -b html -s include/doc-html.dsl driver-guide.sgml
	mkdir -p driver-guide && cd driver-guide && openjade -t sgml -d ../include/doc-html.dsl ../driver-guide.sgml

distclean: clean
	rm -f programmers-guide.pdf
	rm -rf programmers-guide/
	rm -f driver-guide.pdf
	rm -rf driver-guide/

