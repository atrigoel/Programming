#!/bin/sh
# Bootstrap the build system from a fresh checkout.
# Requires: autoconf (and autoheader, part of autoconf).
set -e
autoreconf --install --verbose
echo "Done. Now run: ./configure && make && ./hello"
