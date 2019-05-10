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

# ================ Initialization ==============
# Initialize autoconf: package name, version, bug report address
AC_INIT([zrythm], [0.4.139], [https://git.zrythm.org/zrythm/zrythm/issues])

# needed for localization
AC_DEFINE([GETTEXT_PACKAGE],["zrythm"],
          [Gettext package name])

# =========== Detect OS ================

# AC_CANONICAL_HOST is needed to access the
# 'host_os' variable
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
  cygwin*|mingw*|msys*)
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

# Add GSettings support
GLIB_GSETTINGS

# ========== Check for other conditionals ========

AM_CONDITIONAL([USE_DEFAULT_PREFIX],
               [test "x$prefix" = xNONE])
AM_COND_IF([USE_DEFAULT_PREFIX],
           [AC_SUBST([USE_DEFAULT_PREFIX], [1])])

# ============ Configure options =============

AC_ARG_ENABLE(profiling,
              AS_HELP_STRING([--enable-profiling], [Enables profiling with gprof]),
              [AC_SUBST([PROFILING], [1])],
              [AC_SUBST([PROFILING], [0])])
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug], [Debug build with symbols]),
              [AC_SUBST([DEBUG], [1])],
              [AC_SUBST([DEBUG], [0])])
AC_ARG_WITH(
  docs,
  AS_HELP_STRING(
    [--with-docs],
    [Export documentation]),
  [AC_SUBST([WITH_DOCS],[1])],
  [AC_SUBST([WITH_DOCS],[0])])

AC_ARG_WITH(
  x42-plugins,
  AS_HELP_STRING(
    [--with-x42-plugins],
    [Bundle x42 plugins]),
  [AC_SUBST([WITH_X42_PLUGINS],[1])],
  [AC_SUBST([WITH_X42_PLUGINS],[0])])

AC_ARG_WITH(
  distrho-ports,
  AS_HELP_STRING(
    [--with-distrho-ports],
    [Bundle DISTRHO ports plugins]),
  [AC_SUBST([WITH_DISTRHO_PORTS],[1])],
  [AC_SUBST([WITH_DISTRHO_PORTS],[0])])

# ============= Check for programs ===========

# Check for C/C++ compilers
AC_PROG_CC([gcc clang])
AC_PROG_CXX([g++])

# locale program
AC_CHECK_PROG(LOCALE,[locale],[yes],[no])
test "$LOCALE" == "no" && \
AC_MSG_ERROR(
  [Required program 'locale' not found.]) && \
exit 1

# sed
AC_CHECK_PROG(SED,[sed],[yes],[no])
test "$SED" == "no" && \
AC_MSG_ERROR(
  [Required program 'sed' not found.]) && exit 1

# libtoolize
AM_COND_IF(
  [OSX],
  AC_CHECK_PROG(LIBTOOLIZE,[glibtoolize],[yes],[no])
  test "$LIBTOOLIZE" == "no" && \
  AC_MSG_ERROR(
    [Required program 'glibtoolize' not found.  Please install libtool]) && exit 1,
  AC_CHECK_PROG(LIBTOOLIZE,[libtoolize],[yes],[no])
  test "$LIBTOOLIZE" == "no" && \
  AC_MSG_ERROR(
    [Required program 'libtoolize' not found. Please install libtool]) && exit 1)

# gettext
AC_CHECK_PROG(XGETTEXT,[xgettext],[yes],[no])
test "$XGETTEXT" == "no" && \
AC_MSG_ERROR(
  [Required program 'xgettext' not found.
   Please install gettext]) && exit 1

# ============ Check for libraries ===========

# FFMPEG - optional
AC_ARG_WITH(
  ffmpeg,
  AS_HELP_STRING(
    [--with-ffmpeg], [Add mp3 support via ffmpeg]),
  [PKG_CHECK_MODULES(
    [FFMPEG],
    [libavcodec libavutil libavformat])],
    [echo "** building without FFMPEG (mp3 support). use --with-ffmpeg to enable it"])

# Qt5 - optional
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

# PortAudio - optinal on non-windows
AM_COND_IF(
  [WINDOWS],
  [PKG_CHECK_MODULES(
      [PORT_AUDIO], [portaudio-2.0],
      [AC_DEFINE([HAVE_PORT_AUDIO],[1],[Have port audio backend])
        AC_SUBST([HAVE_PORT_AUDIO],[1])])],
  [AC_ARG_WITH(
    port-audio,
    AS_HELP_STRING(
      [--with-port-audio],
      [Add support for port audio backend]),
    [PKG_CHECK_MODULES(
      [PORT_AUDIO], [portaudio-2.0],
      [AC_DEFINE([HAVE_PORT_AUDIO],[1],[Have port audio backend])
        AC_SUBST([HAVE_PORT_AUDIO],[1])])],
    [echo "** building without port audio support. use --with-port-audio to enable it"])])

# Mandatory libraries
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.20])
PKG_CHECK_MODULES([LILV], [lilv-0 >= 0.24.2])
PKG_CHECK_MODULES([SNDFILE], [sndfile >= 1.0.25])
PKG_CHECK_MODULES(
  [SAMPLERATE], [samplerate >= 0.1.8])
PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])
PKG_CHECK_MODULES([LIBYAML], [yaml-0.1])

# JACK - optional on non-linux
AM_COND_IF(
  [LINUX],
  [PKG_CHECK_MODULES(
     [JACK], [jack],
     [AC_DEFINE([HAVE_JACK],[1],[Have jack backend])
        AC_SUBST([HAVE_JACK],[1])])])

# libgtop - mandatory on windows
AM_COND_IF(
  [WINDOWS],,
  [PKG_CHECK_MODULES(
    [GTOP], [libgtop-2.0],
    [AC_DEFINE([HAVE_LIBGTOP],[1],[Have libgtop])
     AC_SUBST([HAVE_LIBGTOP],[1])],
    [echo "**building without libgtop"])])

# x11 - mandatory on linux
AM_COND_IF(
  [LINUX],
  [PKG_CHECK_MODULES(
     [X11], [x11],
     [AC_DEFINE([HAVE_X11],[1],[Have x11])
        AC_SUBST([HAVE_X11],[1])])])

# x42 plugin dependencies
AS_IF([test "x$with_x42_plugins" == "xyes"],
      [PKG_CHECK_MODULES([FFTW3F], [fftw3f])
       PKG_CHECK_MODULES([GLU], [glu])
       PKG_CHECK_MODULES([_GL], [gl])],
      [have_deps=no])

# ============ Check for header files ==========

# windows.h - mandatory on windows
AM_COND_IF([WINDOWS],
           [AC_CHECK_HEADERS([windows.h])])

# dlfcn.h - mandatory on windows
AM_COND_IF(
  [WINDOWS],
  [AC_CHECK_HEADERS([dlfcn.h],
    [found_dlfcn_header=yes; break;])
  AS_IF([test "x$found_dlfcn_header" != "xyes"],
        [AC_MSG_ERROR([Unable to find dlfcn.h. Please install mingw-w64-dlfcn])])])

# unistd.h
AC_CHECK_HEADERS([unistd.h])

# distrho ports headers
AS_IF(
  [test "x$with_distrho_ports" == "xyes"],
  [AC_CHECK_HEADERS([ladspa.h],
     [found_ladspa_header=yes; break;])
   AS_IF([test "x$found_ladspa_header" != "xyes"],
         [AC_MSG_ERROR(
           [DISTRHO ports require ladspa.h])])])

# =========== Check for library functions ========

# mlock
AC_CHECK_FUNC([mlock],
              [AC_DEFINE([HAVE_MLOCK], [1],
              [Define if mlock exists.])])

# mlock
AC_CHECK_FUNC(
  [jack_set_property],
  [AC_DEFINE([HAVE_JACK_METADATA], [1],
  [Define if jack_set_property exists.])])

# =============== Declare files =============

# Declare output headers
AC_CONFIG_HEADERS([config.h])

# Declare output files
AC_CONFIG_FILES([
  Makefile
  Doxyfile.cfg
  scripts/package_win.sh
  scripts/package_osx.sh
	data/osx/Info.plist
])

# ================= Output ================

# Actually output all declared files
AC_OUTPUT
