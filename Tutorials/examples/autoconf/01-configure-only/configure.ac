# configure.ac -- Autoconf input for the "hello" example (no Automake).
# Regenerate ./configure with:  autoreconf -i

AC_PREREQ([2.69])
AC_INIT([hello-autoconf], [1.0], [bugs@example.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# --- Programs -------------------------------------------------------------
AC_PROG_CC
AC_PROG_INSTALL

# --- Header checks --------------------------------------------------------
AC_CHECK_HEADERS([unistd.h stdint.h])

# --- Library checks -------------------------------------------------------
# sqrt() may require -lm; this appends the needed lib to $LIBS automatically.
AC_SEARCH_LIBS([sqrt], [m], [], [AC_MSG_ERROR([sqrt not found])])

# --- Function checks ------------------------------------------------------
AC_CHECK_FUNCS([getpid])

# --- Custom option: --enable-debug ---------------------------------------
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [build with debug info and no optimization])],
  [debug=$enableval], [debug=no])
AS_IF([test "x$debug" = "xyes"],
  [CFLAGS="$CFLAGS -g -O0"
   AC_DEFINE([ENABLE_DEBUG], [1], [Define to 1 for a debug build])],
  [CFLAGS="$CFLAGS -O2"])

# --- Files to generate ----------------------------------------------------
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# Summary printed at the end of ./configure
echo ""
echo "  $PACKAGE_NAME $PACKAGE_VERSION configured"
echo "    CC      = $CC"
echo "    CFLAGS  = $CFLAGS"
echo "    LIBS    = $LIBS"
echo "    debug   = $debug"
echo ""
