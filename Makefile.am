## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc drivers debian

#m4datadir = $(datadir)/aclocal
#m4data_DATA = dbi.m4

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README README.drivers README.win32 TODO libdbi.spec.in config.h.in Makefile.w32 depcomp
# XXX depcomp is a hack, shouldn't make dist automatically include it??

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
