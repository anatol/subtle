#! /bin/bash
set -x
aclocal
autoheader
automake --add-missing --copy
autoconf
