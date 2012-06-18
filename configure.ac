#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])

# Versions used for apps, plugins, pkg-config files, and data,
# as well as global and user prefs
m4_define([reactor_version], [0.1])
m4_define([reactor_plugin_version], [0.1])
#m4_define([reactor_pkgconfig_version], [0.1])
#m4_define([reactor_data_version], [0.1])
#m4_define([reactor_sysconf_version], [0.1])
#m4_define([reactor_user_version], [0.1])

AC_INIT([reactor], [reactor_version], [vn.alvaro@gmail.com])
AC_CONFIG_SRCDIR([src/reactord.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax subdir-objects dist-bzip2])

AC_CANONICAL_HOST
# Checks for programs.
##AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
#AC_PROG_RANLIB
LT_INIT([])
AC_PREFIX_DEFAULT([/usr])

#Check for _GNU_SOURCE
##AC_USE_SYSTEM_EXTENSIONS

# Checks for libraries.

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.22.0])
#PKG_CHECK_MODULES(LIBEVENT, [libevent >= 2.0.10])
PKG_CHECK_MODULES(CHECK, [check >= 0.9.8])

# Checks for header files.

AC_CHECK_HEADERS(event.h, , AC_MSG_ERROR(Libevent header (event.h) not found. reactor requires libevent to build.))

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

#
# options and defaults
#

AC_ARG_ENABLE(debug,
[  --enable-debug     enable debugging and disable optimization],
[  if test "$enable_debug" = "yes"; then
        DEBUG_CFLAGS="-g3"
        AC_DEFINE(DEBUG, 1, [Define to 1 if you want compile for debugging.])
    else
        DEBUG_CFLAGS="-O2"
    fi
]
)
#AM_CONDITIONAL(ENABLE_DEBUG, test "$enable_debug" = "yes")
CFLAGS="$DEBUG_CFLAGS"

reactorplugindir="$libdir/$PACKAGE/reactor_plugin_version"

AC_SUBST(reactorplugindir)

AS_AC_EXPAND(REACTORPLUGINDIR, $reactorplugindir)

AC_DEFINE_UNQUOTED(REACTORPLUGINDIR, "$REACTORPLUGINDIR", Define where the plugins are located.)

############################################
# libevent checks copied from Tor project. #
############################################
trylibeventdir=""
AC_ARG_WITH(libevent-dir,
       [  --with-libevent-dir=PATH     Specify path to Libevent installation ],
       [
                if test "x$withval" != "xno" ; then
                        trylibeventdir=$withval
                fi
       ]
)

AC_CACHE_CHECK([for libevent directory], ac_cv_libevent_dir, [
  saved_LIBS="$LIBS"
  saved_LDFLAGS="$LDFLAGS"
  saved_CPPFLAGS="$CPPFLAGS"
  le_found=no
  for ledir in $trylibeventdir "" $prefix /usr/local ; do
    LDFLAGS="$saved_LDFLAGS"
    LIBS="$saved_LIBS -levent"

    # Skip the directory if it isn't there.
    if test ! -z "$ledir" -a ! -d "$ledir" ; then
       continue;
    fi
    if test ! -z "$ledir" ; then
      if test -d "$ledir/lib" ; then
        LDFLAGS="-L$ledir/lib $LDFLAGS"
      else
        LDFLAGS="-L$ledir $LDFLAGS"
      fi
      if test -d "$ledir/include" ; then
        CPPFLAGS="-I$ledir/include $CPPFLAGS"
      else
        CPPFLAGS="-I$ledir $CPPFLAGS"
      fi
    fi
    # Can I compile and link it?
    AC_TRY_LINK([#include <sys/time.h>
#include <sys/types.h>
#include <event.h>], [ event_init(); ],
       [ libevent_linked=yes ], [ libevent_linked=no ])
    if test $libevent_linked = yes; then
       if test ! -z "$ledir" ; then
         ac_cv_libevent_dir=$ledir
       else
         ac_cv_libevent_dir="(system)"
       fi
       le_found=yes
       break
    fi
  done
  LIBS="$saved_LIBS"
  LDFLAGS="$saved_LDFLAGS"
  CPPFLAGS="$saved_CPPFLAGS"
  if test $le_found = no ; then
    AC_MSG_ERROR([Could not find a linkable libevent. You can specify an explicit path using --with-libevent-dir])
  fi
])
LIBS="$LIBS -levent"
if test $ac_cv_libevent_dir != "(system)"; then
  if test -d "$ac_cv_libevent_dir/lib" ; then
    LDFLAGS="-L$ac_cv_libevent_dir/lib $LDFLAGS"
    le_libdir="$ac_cv_libevent_dir/lib"
  else
    LDFLAGS="-L$ac_cv_libevent_dir $LDFLAGS"
    le_libdir="$ac_cv_libevent_dir"
  fi
  if test -d "$ac_cv_libevent_dir/include" ; then
    CPPFLAGS="-I$ac_cv_libevent_dir/include $CPPFLAGS"
  else
    CPPFLAGS="-I$ac_cv_libevent_dir $CPPFLAGS"
  fi
fi

AC_CACHE_CHECK([whether we need extra options to link libevent],
               ac_cv_libevent_linker_option, [
    saved_LDFLAGS="$LDFLAGS"
    le_runs=no
    linked_with=nothing
    for le_extra in "" "-Wl,-R$le_libdir" "-R$le_libdir" ; do
      LDFLAGS="$le_extra $saved_LDFLAGS"
      AC_TRY_RUN([void *event_init(void);
                int main(int c, char **v) {
                   event_init(); return 0;
                }],
                libevent_runs=yes, libevent_runs=no)
      if test $libevent_runs = yes ; then
        if test -z "$le_extra" ; then
          ac_cv_libevent_linker_option='(none)'
        else
          ac_cv_libevent_linker_option=$le_extra
        fi
        le_runs=yes
        break
      fi
    done
    if test $le_runs = no ; then
      AC_MSG_ERROR([Found linkable libevent in $ac_cv_libevent_dir, but it doesn't run, even with -R.  Maybe specify another using --with-libevent-dir?])
    fi
    LDFLAGS="$saved_LDFLAGS"
])

if test $ac_cv_libevent_linker_option != '(none)' ; then
   LDFLAGS="$ac_cv_libevent_linker_option $LDFLAGS"
fi

AC_CONFIG_FILES([Makefile lib/libreactor.pc])
AC_OUTPUT