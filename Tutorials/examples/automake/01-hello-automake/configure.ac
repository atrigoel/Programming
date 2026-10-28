AC_PREREQ([2.69])
AC_INIT([hello-automake], [1.0], [bugs@example.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])

# 'foreign' relaxes the GNU requirement for NEWS/AUTHORS/etc.
# 'subdir-objects' places .o files next to their sources.
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_RANLIB          # needed for the static convenience library

AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  src/Makefile
  tests/Makefile
])
AC_OUTPUT
