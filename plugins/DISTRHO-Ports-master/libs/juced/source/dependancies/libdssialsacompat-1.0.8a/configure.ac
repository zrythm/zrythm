dnl Process this file with autoconf to produce a configure script.

AC_INIT(libdssialsacompat, 1.0.8a, musound-at-jps-dot-net)

AC_CONFIG_SRCDIR(dssi_alsa_compat.c)
AM_INIT_AUTOMAKE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AM_PROG_LIBTOOL

dnl Use lotsa flags if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
  case " $CFLAGS " in
  *[\ \	]-O[1-9][\ \	]*) ;;
  *) CFLAGS="$CFLAGS -O2 -fomit-frame-pointer -funroll-all-loops -finline-functions -ffast-math" ;;
  esac
fi
changequote([,])dnl
 
AC_OUTPUT([
Makefile
libdssialsacompat.pc
])

