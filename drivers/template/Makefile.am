AUTOMAKE_OPTIONS = foreign

#if HAVE_FOOBAR

template_ltlibs = libtemplate.la
template_ldflags = -export-dynamic -avoid-version
template_sources = dbd_template.c

#else
#
#template_ltlibs =
#template_ldflags =
#template_sources =
#
#endif


INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include

libdir = $(plugindir)
lib_LTLIBRARIES = $(template_ltlibs)
libtemplate_la_LDFLAGS = $(template_ldflags)
libtemplate_la_SOURCES = $(template_sources)

EXTRA_DIST = dbd_template.c

#clean:
#	rm -f *.o *.lo *.la
#	rm -rf .libs/ .deps/
#
#distclean: clean
#	rm -f Makefile Makefile.in
