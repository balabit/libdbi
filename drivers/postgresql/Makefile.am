AUTOMAKE_OPTIONS = foreign

if HAVE_PGSQL

pgsql_ltlibs = libpgsql.la
pgsql_ldflags = -export-dynamic -avoid-version
pgsql_sources = dbd_pgsql.c

else

pgsql_ltlibs =
pgsql_ldflags =
pgsql_sources =

endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include @PGSQL_INCLUDE@

libdir = $(plugindir)
lib_LTLIBRARIES = $(pgsql_ltlibs)
libpgsql_la_LDFLAGS = $(pgsql_ldflags) @PGSQL_LFLAGS@ @PGSQL_LIBS@
libpgsql_la_SOURCES = $(pgsql_sources)
libpgsql_la_DEPENDENCIES = $(top_srcdir)/include/dbi/dbd.h $(top_srcdir)/include/dbi/dbi-dev.h $(top_srcdir)/include/dbi/dbi.h pgsql-stuff.h

EXTRA_DIST = dbd_pgsql.c pgsql-stuff.h
