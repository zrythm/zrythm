#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([libsmf],[1.3],[trasz@FreeBSD.org])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([src/smf.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [pow])
AC_ARG_WITH([readline],
	    [AS_HELP_STRING([--with-readline],
	    [support fancy command line editing @<:@default=check@:>@])],
	    [],
	    [with_readline=check])

AS_IF([test "x$with_readline" != xno],
      [AC_CHECK_LIB([readline], [main],
		    [AC_SUBST([READLINE_LIBS], ["-lreadline -lncurses"])
		    AC_DEFINE([HAVE_LIBREADLINE], [1], [Define if you have libreadline])],
		    [if test "x$with_readline" != xcheck; then
		     AC_MSG_FAILURE(
				    [--with-readline was given, but test for readline failed])
		     fi
		     ], -lncurses)])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset pow strdup strerror strtol strchr])

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.2)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AC_CONFIG_FILES([Makefile smf.pc src/Makefile man/Makefile])
AC_OUTPUT
