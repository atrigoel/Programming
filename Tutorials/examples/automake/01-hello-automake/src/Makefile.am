# Build and install the 'hello' program; link the convenience lib.
bin_PROGRAMS = hello
hello_SOURCES = main.c

# Find util.h in ../lib (works for out-of-tree builds via top_srcdir).
AM_CPPFLAGS = -I$(top_srcdir)/lib

# Link the static convenience library built in ../lib.
hello_LDADD = $(top_builddir)/lib/libutil.a
