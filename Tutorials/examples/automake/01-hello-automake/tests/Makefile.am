# check_PROGRAMS are built only for `make check`, never installed.
check_PROGRAMS = test_util
test_util_SOURCES = test_util.c
test_util_CPPFLAGS = -I$(top_srcdir)/lib
test_util_LDADD = $(top_builddir)/lib/libutil.a

# Programs/scripts to execute; a non-zero exit means failure.
TESTS = test_util
