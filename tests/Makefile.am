## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

TESTS = test_dbi
check_PROGRAMS = test_dbi
test_dbi_SOURCES = test_dbi.c

test_dbi_LDADD = -lm -ldbi
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include
CFLAGS = -L$(top_srcdir)/src/.libs -DDBI_DRIVER_DIR=\"@driverdir@\"

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
