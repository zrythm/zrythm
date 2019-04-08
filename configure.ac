#
#  Copyright (C) 2018-2019 Alexandros Theodotou
#
#  This file is part of Zrythm
#
#  Zrythm is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Zrythm is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
#
# Initialize autoconf: package name, version, bug report address
AC_INIT([zrythm], [0.3.029], [https://git.zrythm.org/zrythm/zrythm/issues])

# Declare config.h as output header
AC_CONFIG_HEADERS([config.h])

# AC_CANONICAL_HOST is needed to access the 'host_os' variable
AC_CANONICAL_HOST

build_linux=no
build_freebsd=no
build_windows=no
build_mac=no

# Detect the target system
case "${host_os}" in
  linux*)
    build_linux=yes
    ;;
  freebsd*)
    build_freebsd=yes
    ;;
  cygwin*|mingw*)
    build_windows=yes
    ;;
  darwin*)
    build_mac=yes
    ;;
  *)
    AC_MSG_ERROR(["OS $host_os is not supported"])
    ;;
esac

# Pass the conditionals to automake
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_COND_IF([LINUX],
           [AC_SUBST([LINUX], [1])])
AM_CONDITIONAL([FREEBSD], [test "$build_freebsd" = "yes"])
AM_COND_IF([FREEBSD],
           [AC_SUBST([FREEBSD], [1])])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])
AM_COND_IF([WINDOWS],
           [AC_SUBST([WINDOWS], [1])])
AM_CONDITIONAL([OSX], [test "$build_mac" = "yes"])
AM_COND_IF([OSX],
           [AC_SUBST([OSX], [1])])
AM_CONDITIONAL([USE_DEFAULT_PREFIX],
               [test "x$prefix" = xNONE])
AM_COND_IF([USE_DEFAULT_PREFIX],
           [AC_SUBST([USE_DEFAULT_PREFIX], [1])])

# options

AC_ARG_ENABLE(profiling,
              AS_HELP_STRING([--enable-profiling], [Enables profiling with gprof]),
              [AC_SUBST([PROFILING], [1])],
              [AC_SUBST([PROFILING], [0])])
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug], [Debug build with symbols]),
              [AC_SUBST([DEBUG], [1])],
              [AC_SUBST([DEBUG], [0])])

AC_ARG_WITH(
  ffmpeg,
  AS_HELP_STRING(
    [--with-ffmpeg], [Add mp3 support via ffmpeg]),
  [PKG_CHECK_MODULES(
    [FFMPEG],
    [libavcodec libavutil libavformat])],
    [echo "** building without FFMPEG (mp3 support). use --with-ffmpeg to enable it"])

AC_ARG_WITH(
  qt5,
  AS_HELP_STRING(
    [--with-qt5],
    [Add support for embedding Qt5 plugin UIs]),
  [PKG_CHECK_MODULES(
    [QT5], [Qt5Widgets],
    [AC_DEFINE([HAVE_QT5],[1],[Qt5 support in suil])
      AC_SUBST([HAVE_QT5],[1])])],
  [echo "** building without Qt5 support in suil. use --with-qt5 to enable it"])

AC_ARG_WITH(
  port-audio,
  AS_HELP_STRING(
    [--with-port-audio],
    [Add support for port audio backend]),
  [PKG_CHECK_MODULES(
    [PORT_AUDIO], [portaudio-2.0],
    [AC_DEFINE([HAVE_PORT_AUDIO],[1],[Have port audio backend])
      AC_SUBST([HAVE_PORT_AUDIO],[1])])],
  [echo "** building without port audio support. use --with-port-audio to enable it"])

# needed for localization
AC_DEFINE([GETTEXT_PACKAGE],["zrythm"],
          [Gettext package name])

# FIXME maybe not needed
AC_ARG_WITH(
  docs,
  AS_HELP_STRING(
    [--with-docs],
    [Export documentation]),
  [AC_SUBST([WITH_DOCS],[1])],
  [AC_SUBST([WITH_DOCS],[0])])

# Check for a C compiler
AC_PROG_CC([gcc])
AC_PROG_CXX([g++])

AC_PREFIX_DEFAULT([/usr])

#Checks for libraries.
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.20])
PKG_CHECK_MODULES([JACK], [jack])
PKG_CHECK_MODULES([LILV], [lilv-0 >= 0.24.2])

#PKG_CHECK_MODULES([LIBDAZZLE], [libdazzle-1.0 >= 3.20])
PKG_CHECK_MODULES([SNDFILE], [sndfile >= 1.0.25])
PKG_CHECK_MODULES([SAMPLERATE], [samplerate >= 0.1.8])
AM_COND_IF([LINUX],
           [PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])])
AM_COND_IF([WINDOWS],
           [PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])])
#PKG_CHECK_MODULES([LIBCYAML], [libcyaml])
PKG_CHECK_MODULES([LIBYAML], [yaml-0.1])
PKG_CHECK_MODULES([X11], [x11])
#PKG_CHECK_MODULES([LIBGTOP], [libgtop-2.0])
#PKG_CHECK_MODULES([QT4], [QtGui])


#Checks for header files.
#Checks for typedefs, structures, and compiler characteristics.
#Checks for library functions.
AM_COND_IF([WINDOWS],
           [AC_CHECK_HEADERS([windows.h])])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_FUNC([mlock],
              [AC_DEFINE([HAVE_MLOCK], [1],
              [Define if mlock exists.])])
AC_CHECK_PROG(LOCALE,[locale],[yes],[no])
test "$LOCALE" == "no" && AC_MSG_ERROR([Required program 'locale' not found.]) && exit 1
AC_CHECK_PROG(SED,[sed],[yes],[no])
test "$SED" == "no" && AC_MSG_ERROR([Required program 'sed' not found.]) && exit 1
AC_CHECK_PROG(LIBTOOLIZE,[libtoolize],[yes],[no])
test "$LIBTOOLIZE" == "no" && AC_MSG_ERROR([Required program 'libtoolize' not found. libtool must be installed.]) && exit 1
AC_CHECK_PROG(XGETTEXT,[xgettext],[yes],[no])
test "$XGETTEXT" == "no" && AC_MSG_ERROR([Required program 'xgettext' not found. gettext must be installed.]) && exit 1

#AM_COND_IF([LINUX],
           #[LDFLAGS="$LDFLAGS -L/usr/lib"])

# Check for breeze icons
AM_COND_IF([USE_DEFAULT_PREFIX],
           [AC_CHECK_FILE([$ac_default_prefix/share/icons/breeze-dark],,
              [echo "Breeze icons are required to show the icons properly" && \
               echo "Please install them using a package manager or manually from here https://github.com/KDE/breeze-icons"])],
           [AC_CHECK_FILE([$prefix/share/icons/breeze-dark],,
              [echo "Breeze icons are required" && \
               echo "Please install them using a package manager or manually from here https://github.com/KDE/breeze-icons"])])

# Add GSettings support
GLIB_GSETTINGS

# Declare output files
AC_CONFIG_FILES([
  Makefile
  Doxyfile.cfg
])

# Actually output all declared files
AC_OUTPUT
