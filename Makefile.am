## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc plugins

#m4datadir = $(datadir)/aclocal
#m4data_DATA = ao.m4

EXTRA_DIST = 

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

#clean:
#
#distclean: clean
#	rm -f config.status config.cache config.log configure
#	rm -f Makefile Makefile.in
