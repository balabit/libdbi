## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

doc_DATA = programmers-guide.pdf programmers-guide/* plugin-guide.pdf plugin-guide/*

EXTRA_DIST = $(doc_DATA)

# jadetex spews out tons of useless crap. shut it up:
programmers-guide.pdf:
	@echo "Auto-generating Programmer's Guide reference SGML...";
	./include/markup_to_docbook.pl libdbi-reference.tmpl api-documentation.src
	@echo "Converting programmers-guide.sgml to PostScript...";
	sgmltools -b ps -s include/doc-print.dsl programmers-guide.sgml > /dev/null 2>&1
	@echo "Converting programmers-guide.ps to PDF...";
	ps2pdf programmers-guide.ps

programmers-guide/*:
	@echo "Converting programmers-guide.sgml to HTML...";
	sgmltools -b html -s include/doc-html.dsl programmers-guide.sgml

plugin-guide.pdf:
	@echo "Auto-generating Plugin Author's Guide reference SGML...";
	./include/markup_to_docbook.pl plugin-guide.tmpl plugin-documentation.src
	@echo "Converting plugin-guide.sgml to PostScript...";
	sgmltools -b ps -s include/doc-print.dsl plugin-guide.sgml > /dev/null 2>&1
	@echo "Converting plugin-guide.ps to PDF...";
	ps2pdf plugin-guide.ps

plugin-guide/*:
	@echo "Converting plugin-guide.sgml to HTML...";
	sgmltools -b html -s include/doc-html.dsl plugin-guide.sgml

distclean: clean
	rm -f programmers-guide.ps
	if [ -f programmers-guide.tmpl ]; then rm -f programmers-guide/ programmers-guide.pdf; fi
	rm -f plugin-guide.ps
	if [ -f plugin-guide.tmpl ]; then rm -f plugin-guide/ plugin-guide.pdf; fi

# this can't be kosher...
install-exec-am: $(top_srcdir)/mkinstalldirs
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(docdir) $(DESTDIR)$(docdir)/programmers-guide
