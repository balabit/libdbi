AUTOMAKE_OPTIONS = foreign

if HAVE_MYSQL

mysql_ltlibs = libmysql.la
mysql_ldflags = -export-dynamic -avoid-version
mysql_sources = dbd_mysql.c

else

mysql_ltlibs =
mysql_ldflags =
mysql_sources =

endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include ${MYSQL_INCLUDE}

libdir = $(plugindir)
lib_LTLIBRARIES = $(mysql_ltlibs)
libmysql_la_LDFLAGS = $(mysql_ldflags) ${MYSQL_LFLAGS} ${MYSQL_LIBS}
libmysql_la_SOURCES = $(mysql_sources)

EXTRA_DIST = dbd_mysql.c
