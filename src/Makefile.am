## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

LIBS = -lm # make sure this is right
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include

lib_LTLIBRARIES = libdbi.la

libdbi_la_SOURCES = dbi_main.c dbi_result.c dbd_helper.c atoll.c
libdbi_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

CFLAGS = -DDBI_PLUGIN_DIR=\"@plugindir@\"

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
