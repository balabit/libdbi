## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

LIBS = -lm
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include

lib_LTLIBRARIES = libdbi.la

libdbi_la_SOURCES = dbi_main.c dbi_result.c dbd_helper.c atoll.c asprintf.c timegm.c
libdbi_la_LIBADD = $(LIBADD_DL)
libdbi_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@ -no-undefined

CFLAGS = -DDBI_DRIVER_DIR=\"@driverdir@\"

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
