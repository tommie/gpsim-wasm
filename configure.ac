#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([gpsim],[0.31.1],[<http://gpsim.sourceforge.net/gpsim.html>])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR([m4])

# Determine the host and build type. The target is always a PIC.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

dnl --enable-leak-sanitize : Turn on memory leak detection
dnl The default is off
dnl X86-64 Linux GCC 4.8+ CLang only

AC_ARG_ENABLE(leak-sanitize,
    [  --enable-leak-sanitize    Enable memory leak debugging],
     [case "${enableval}" in
      yes) use_leak_sanitize=yes ;;
      no) use_leak_sanitize=no ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --enable-leak-sanitize) ;;
    esac],[use_leak_sanitize=no])

if test "$use_leak_sanitize" = "yes"; then
    echo enabling memory leak debugging
    LD_SANITIZE="-fsanitize=leak"
else
    LD_SANITIZE=""
fi

dnl --enable-address-sanitize : Turn on memory error detection
dnl The default is off
dnl X86-64 Linux GCC 4.8+ CLang only

AC_ARG_ENABLE(address-sanitize,
    [  --enable-address-sanitize    Enable memory error debugging],
     [case "${enableval}" in
      yes) use_address_sanitize=yes ;;
      no) use_address_sanitize=no ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --enable-address-sanitize) ;;
    esac],[use_address_sanitize=no])

if test "$use_address_sanitize" = "yes"; then
    echo enabling memory address debugging
    LD_ADDRESS="-fsanitize=address"
else
    LD_ADDRESS=""
fi

dnl --enable-undefined-sanitize : Turn on undefined behavior detection
dnl The default is off
dnl X86-64 Linux GCC 4.8+ CLang only

AC_ARG_ENABLE(undefined-sanitize,
    [  --enable-undefined-sanitize    Enable undefined behavior detection],
     [case "${enableval}" in
      yes) use_undefined_sanitize=yes ;;
      no) use_undefined_sanitize=no ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --enable-undefined-sanitize) ;;
    esac],[use_undefined_sanitize=no])

if test "$use_undefined_sanitize" = "yes"; then
    echo enabling undefined behavior detection
    LD_UNDEFINED="-fsanitize=undefined"
else
    LD_UNDEFINED=""
fi

dnl --disable-gui : turn off gui support (cli only)
dnl    The default is to have the gui.

AC_ARG_ENABLE(gui,
     [  --disable-gui           Only use the cli and not the gui],
     [case "${enableval}" in
       yes) use_gui=yes ;;
       no)  use_gui=no ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --disable-gui) ;;
     esac],[use_gui=yes])

if test "$use_gui" = "no"; then
        echo disabling gui support
else
        echo enabling gui support
        [
         use_gui=yes
        ]
        AC_DEFINE([HAVE_GUI],[],[True if GUI is being used])
fi


dnl --enable-sockets : allows gpsim to be controlled from a socket interface
dnl    The default is to not use sockets.

AC_ARG_ENABLE(sockets,
     [  --enable-sockets        Allows gpsim to be controlled via a socket interface],
     [case "${enableval}" in
       yes) use_sockets=yes ;;
       no)  use_sockets=no ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-sockets) ;;
     esac],[use_sockets=no])

if test "$use_sockets" = "no"; then
        echo disabling gpsim socket interface
else
        echo enabling gpsim socket interface
        [
         use_sockets=yes
        ]
        AC_DEFINE([HAVE_SOCKETS],[],[True if gpsim socket interface is being used])
fi

dnl --disable-cli : turn off cli support
dnl    The default is to have the cli.

AC_ARG_ENABLE(cli,
     [  --disable-cli           Do not build the cli],
     [case "${enableval}" in
       yes) use_cli=yes ;;
       no)  use_cli=no ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --disable-cli) ;;
     esac],[use_cli=yes])

if test "$use_cli" = "no"; then
        echo disabling cli support
else
        echo enabling cli support
        [
         use_cli=yes
        ]
        AC_DEFINE([HAVE_CLI],[],[True if CLI is being used])
fi
AM_CONDITIONAL([HAVE_CLI],[test x$use_cli = xyes])
AM_CONDITIONAL([HAVE_PROGRAM],[test x$use_cli = xyes -o x$use_gui = xyes])

GTK=
GDK=
GLIB=

PKG_PROG_PKG_CONFIG()

if test "$use_gui" = "no"; then
  dnl glib2 checks

  if test "$use_cli" = "yes"; then
    PKG_CHECK_MODULES(P_GLIB, [glib-2.0 >= 2.26 gthread-2.0 gmodule-2.0])
  fi
  X_LDFLAGS=$P_GLIB_LIBS
  X_CFLAGS=$P_GLIB_CFLAGS
  Y_LDFLAGS=
  Y_CFLAGS=
else
  dnl gtk2 checks

#  PKG_CHECK_MODULES(GTKEXTRAMOD, gtkextra-2.0, ,
#    [PKG_CHECK_MODULES(GTKEXTRAMOD, gtkextra-3.0, ,
#      [AC_MSG_ERROR(Cannot find gtkextra-2.0 or gtkextra-3.0 package)])])

  PKG_CHECK_MODULES(P_GTK, [gtk+-2.0 >= 2.24 glib-2.0 >= 2.26 gthread-2.0])
  X_LDFLAGS=$P_GTK_LIBS
  X_CFLAGS=$P_GTK_CFLAGS
#  Y_LDFLAGS=$GTKEXTRAMOD_LIBS
#  Y_CFLAGS=$GTKEXTRAMOD_CFLAGS
  GTK_VERSION_T=`$PKG_CONFIG --modversion gtk+-2.0`
  echo linking with gtk-$GTK_VERSION_T
  AC_DEFINE_UNQUOTED([GTK_VERSION],"$GTK_VERSION_T",[gtk version])
fi

wi_LIB_READLINE
if test "$use_cli" = "yes"; then
  if test "$wi_cv_lib_readline" != yes; then
	AC_MSG_ERROR(Cannot find readline library)
  fi
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC
AM_PROG_LEX
LT_INIT

# Checks for libraries.
AC_LANG([C++])
#AC_CHECK_LIB([gpsim], [main])
#AC_CHECK_LIB([pthread], [main])

if test "$use_cli" = "yes"; then
  dnl check if popt is installed
  AC_CHECK_HEADER(popt.h, , AC_MSG_ERROR(popt not installed: cannot find popt.h))
  AC_CHECK_LIB([popt], [main])
fi

AC_CHECK_LIB([dl], [dlopen], [LIBDL="-ldl"])

# Checks for header files.
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
m4_warn([obsolete],
[Update your code to rely only on HAVE_SYS_TIME_H,
then remove this warning and the obsolete code below it.
All current systems provide time.h; it need not be checked for.
Not all systems provide sys/time.h, but those that do, all allow
you to include it and time.h simultaneously.])dnl
AC_CHECK_HEADERS_ONCE([sys/time.h])
# Obsolete code to be removed.
if test $ac_cv_header_sys_time_h = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],[Define to 1 if you can safely include both <sys/time.h>
	     and <time.h>.  This macro is obsolete.])
fi
# End of obsolete code.

m4_warn([obsolete],
[your code may safely assume C89 semantics that RETSIGTYPE is void.
Remove this warning and the `AC_CACHE_CHECK' when you adjust the code.])dnl
AC_CACHE_CHECK([return type of signal handlers],[ac_cv_type_signal],[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
],
		 [return *(signal (0, 0)) (0) == 1;])],
		   [ac_cv_type_signal=int],
		   [ac_cv_type_signal=void])])
AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],[Define as the return type of signal handlers
		    (`int' or `void').])


# Checks for library functions.
AC_FUNC_ALLOCA
AC_PROG_GCC_TRADITIONAL
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor gethostbyname gethostname gettimeofday memset pow select socket sqrt strcasecmp strchr strdup strerror strncasecmp strndup strpbrk strrchr strstr strtoul])

# printf modifier define for long long as "ll"
# config_win32.h.in defines this for Visual Studio stdclib as "I64"
AC_DEFINE([PRINTF_INT64_MODIFIER],["ll"],[printf modifier define for long long])

# define printf modifier for GINT64 (guint64 and gint64) as "ll" on 32 bit machines and as "l" on 64 bit machines
# config_win32.h.in defines this for Visual Studio stdclib as "I64"
AC_CHECK_SIZEOF(long)
if test "$ac_cv_sizeof_long" -ge 8; then
  gpsim_cv_printf_gint64_modifier=l
else
  gpsim_cv_printf_gint64_modifier=ll
fi
AC_DEFINE_UNQUOTED([PRINTF_GINT64_MODIFIER],"$gpsim_cv_printf_gint64_modifier",[printf modifier define for GINT64])

AM_CFLAGS=
AM_CXXFLAGS=
AM_LDFLAGS=

# Options for the system on which the package will run
case "${host}" in
  *linux* )
    if test "x$GCC" = "xyes"; then
      AM_CFLAGS="-Wall"
      AM_CXXFLAGS="-Wall -std=c++11"
      AM_LDFLAGS="-Wl,-warn-common -Wl,-warn-once"
    fi
    ;;
  *mingw* )
    ;;
esac

CFLAGS="${CFLAGS} ${AM_CFLAGS} ${LD_SANITIZE} ${LD_ADDRESS} ${LD_UNDEFINED}"
CXXFLAGS="${CXXFLAGS} ${AM_CXXFLAGS} ${LD_SANITIZE} ${LD_ADDRESS} ${LD_UNDEFINED}"
LDFLAGS="${LDFLAGS} ${AM_LDFLAGS} ${LD_SANITIZE} ${LD_ADDRESS} ${LD_UNDEFINED}"

# Host filesystem options
case "${host}" in
  *mingw* | *-pc-os2_emx | *-pc-os2-emx | *djgpp* )
    AC_DEFINE(HAVE_DOS_BASED_FILE_SYSTEM, 1,
      [Define if your host uses a DOS based file system. ])
    ;;
esac

AC_SUBST(Y_CFLAGS)
AC_SUBST(Y_LDFLAGS)
AC_SUBST(X_CFLAGS)
AC_SUBST(X_LDFLAGS)
AC_SUBST(GTK)
AC_SUBST(GDK)
AC_SUBST(GLIB)
AC_SUBST(LIBREADLINE)
AC_SUBST(LIBDL)

AC_CONFIG_FILES([Makefile
                 cli/Makefile
                 doc/Makefile
                 examples/Makefile
                 examples/modules/Makefile
                 examples/projects/Makefile
                 examples/12bit/Makefile
                 examples/14bit/Makefile
                 examples/16bit/Makefile
                 eXdbm/Makefile
                 gpsim/Makefile
                 gui/Makefile
                 modules/Makefile
		 extras/Makefile
		 extras/lcd/Makefile
		 extras/dht11/Makefile
		 extras/ds1820/Makefile
		 extras/ds1307/Makefile
		 extras/solar/Makefile
		 extras/graphic_lcd/Makefile
                 regression/Makefile
                 src/Makefile
                 src/dspic/Makefile
                 xpms/Makefile
                 gpsim.spec])

AC_OUTPUT

AC_MSG_RESULT([

gpsim-$PACKAGE_VERSION is now configured for $canonical_host_type

  Build:                $build
  Host:                 $host
  Source directory:     $srcdir
  Installation prefix:  $prefix
  C compiler:           $CC $CPPFLAGS $CFLAGS
  C++ compiler:         $CXX $CPPFLAGS $CXXFLAGS

  gui:                  $use_gui
  Socket interface:     $use_sockets

])
