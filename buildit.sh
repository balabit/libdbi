#!/bin/bash
aclocal
automake --add-missing
autoconf
echo "Done, ready to ./configure && make dist."
