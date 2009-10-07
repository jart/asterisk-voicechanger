#!/bin/sh
#
# The purpose of this script is to generate all the autotools files
# from a fresh mercurial clone so you can actually build the darn
# thing.
#

set -x

if [ -f Makefile ]; then
    make maintainer-clean      || exit $?
fi

autoreconf --install --verbose || exit $?

# aclocal -I m4                  || exit $?
# libtoolize --force --copy      || exit $?
# autoheader                     || exit $?
# automake --add-missing --copy  || exit $?
# autoconf                       || exit $?

