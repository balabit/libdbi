## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc tests

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README README.drivers README.win32 README.osx TODO libdbi.spec.in config.h.in Makefile.w32 autogen.sh

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
