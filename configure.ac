# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([Multitouch Protocol Translation Library],
        [1.0.3],
        [],
        [mtdev])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

LIB_VERSION=1:0:0
AC_SUBST([LIB_VERSION])

# Initialize libtool
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Optional test build
AC_ARG_ENABLE(test,
              AC_HELP_STRING([--enable-test],
	      [Build test programs [[default=auto]]]),
              [build_test="$enableval"],
              [build_test="auto"])
AM_CONDITIONAL(BUILD_TEST, [ test "$build_test" != "no" ])

# Initialize X.Org macros
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.5 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.5)
XORG_DEFAULT_OPTIONS

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 mtdev.pc])
AC_OUTPUT
