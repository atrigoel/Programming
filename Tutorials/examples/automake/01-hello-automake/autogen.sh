#!/bin/sh
# Bootstrap: generate configure, Makefile.in files, and install helper scripts.
# Requires: autoconf, automake.
set -e
mkdir -p build-aux m4
autoreconf --install --verbose
echo "Done. Now: mkdir build && cd build && ../configure && make && make check"
