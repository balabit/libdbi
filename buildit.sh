#!/bin/bash
libtoolize --force --copy
aclocal
autoheader
automake --add-missing
autoconf
echo "Done, ready to ./configure && make dist."
