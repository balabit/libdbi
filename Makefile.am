## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README README.win32 README.osx TODO NEWS libdbi.spec.in config.h.in Makefile.w32 autogen.sh

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
