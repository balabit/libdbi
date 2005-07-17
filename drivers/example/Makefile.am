AUTOMAKE_OPTIONS = foreign

if HAVE_EXAMPLE

# don't use docdir, doc_DATA. This way we keep the HTML in separate subdirs
# for each driver
doc_dir = $(datadir)/doc/$(PACKAGE)-$(VERSION)
html_subdir = dbd_example
noinst_DATA = dbd_example.pdf dbd_example/* 

example_ltlibs = libexample.la
example_ldflags = -module -avoid-version
example_sources = dbd_example.c

else

doc_dir =
html_subdir =
noinst_DATA =

example_ltlibs =
example_ldflags =
example_sources =

endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include @DBI_INCLUDE@ @EXAMPLE_INCLUDE@

libdir = $(driverdir)
lib_LTLIBRARIES = $(example_ltlibs)
libexample_la_LDFLAGS = $(example_ldflags) @EXAMPLE_LDFLAGS@ @EXAMPLE_LIBS@
libexample_la_SOURCES = $(example_sources)
libexample_la_DEPENDENCIES = dbd_example.h

EXTRA_DIST = dbd_example.c dbd_example.h AUTHORS TODO README dbd_example.pdf dbd_example/*

driverdoc_sources = dbd_example.sgml

dbd_example.pdf: $(driverdoc_sources)
	@echo "Converting dbd_example.sgml to PDF...";
	openjade -t tex -V tex-backend -d $(top_srcdir)/doc/include/doc-print.dsl dbd_example.sgml
# run jadetex three times to get the references right
	pdftex "&pdfjadetex" dbd_example.tex
	pdftex "&pdfjadetex" dbd_example.tex
	pdftex "&pdfjadetex" dbd_example.tex

dbd_example/*:
	@echo "Converting dbd_example.sgml to HTML...";
	mkdir -p dbd_example && cd dbd_example && openjade -t sgml -d $(top_srcdir)/doc/include/doc-html.dsl ../dbd_example.sgml


distclean: clean
	rm -f dbd_example.pdf dbd_example/*

# manually install the docs
install-data-local: 
if HAVE_EXAMPLE
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(doc_dir) $(DESTDIR)$(doc_dir)/$(html_subdir)
	@for f in $(noinst_DATA); do \
		$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(doc_dir)/$$f; \
	done
endif