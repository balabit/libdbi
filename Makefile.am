## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README README.win32 README.osx TODO NEWS libdbi.spec.in config.h.in Makefile.w32 autogen.sh drivers/example/AUTHORS drivers/example/Makefile.am drivers/example/README drivers/example/TODO drivers/example/.cvsignore drivers/example/dbd_example.c drivers/example/dbd_example.h drivers/example/dbd_example.sgml

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
