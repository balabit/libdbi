#!/bin/bash
export LANG=en_US
aclocal
automake --add-missing
autoconf
autoheader
echo "Done, ready to ./configure && make dist."
