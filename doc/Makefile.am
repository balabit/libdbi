## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

doc_DATA = programmers-guide.pdf programmers-guide/*

EXTRA_DIST = $(doc_DATA)

# jadetex spews out tons of useless crap. shut it up:
programmers-guide.pdf:
	@echo "Converting programmers-guide.sgml to PostScript...";
	sgmltools -b ps -s include/doc-print.dsl programmers-guide.sgml > /dev/null 2>&1
	@echo "Converting programmers-guide.ps to PDF...";
	ps2pdf programmers-guide.ps

programmers-guide/*:
	@echo "Converting programmers-guide.sgml to HTML...";
	sgmltools -b html -s include/doc-html.dsl programmers-guide.sgml

distclean: clean
	rm -rf programmers-guide/
	rm -f programmers-guide.ps programmers-guide.pdf

install:
	test -d $(docdir)/programmers-guide || mkdir $(docdir)/programmers-guide || exit 1
