#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([reactor], [0.0], [vn.alvaro@gmail.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax subdir-objects dist-bzip2])

AC_CANONICAL_HOST

# Checks for programs.
##AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

#Check for _GNU_SOURCE
##AC_USE_SYSTEM_EXTENSIONS

# Checks for libraries.

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.22.0])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

#
# options and defaults
#

AC_ARG_ENABLE(debug,
[  --enable-debug           specify you are building with debugging on],
    AC_DEFINE(DEBUG, 1, [Define if you want compile for debugging.]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT