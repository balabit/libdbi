## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc drivers tests

#m4datadir = $(datadir)/aclocal
#m4data_DATA = dbi.m4

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README README.drivers README.win32 TODO libdbi.spec.in config.h.in Makefile.w32

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
