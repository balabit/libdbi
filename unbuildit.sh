#!/bin/bash
find . -name Makefile.in -exec rm -v {} \;
rm aclocal.m4 config.guess config.sub configure install-sh ltconfig ltmain.sh missing mkinstalldirs
