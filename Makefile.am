## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc plugins debian

#m4datadir = $(datadir)/aclocal
#m4data_DATA = dbi.m4

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README README.plugins TODO libdbi.spec

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
