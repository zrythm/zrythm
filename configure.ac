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
AC_INIT([zrythm], [0.0.1], [alex@zrythm.org])

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
AC_ARG_ENABLE(package-mode,
              AS_HELP_STRING([--enable-package-mode], [Used when making deb/rpm/pkg packages]),
              [AC_SUBST([PACKAGE_MODE], [1])],
              [AC_SUBST([PACKAGE_MODE], [0])])

AC_ARG_ENABLE(profiling,
              AS_HELP_STRING([--enable-profiling], [Enables profiling with gprof]),
              [AC_SUBST([PROFILING], [1])],
              [AC_SUBST([PROFILING], [0])])

AC_ARG_ENABLE(release,
              AS_HELP_STRING([--enable-release], [Build for release (default is debug)]),
              [AC_SUBST([RELEASE], [1])]
              [AC_DEFINE([RELEASE], [1], [Compiled in release mode])],
              [AC_SUBST([RELEASE], [0])])

# Check for a C compiler
AC_PROG_CC([gcc])

AC_PREFIX_DEFAULT([/usr])

#Checks for libraries.
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.20])
PKG_CHECK_MODULES([JACK], [jack])
PKG_CHECK_MODULES([LILV], [lilv-0 >= 0.24.2])
PKG_CHECK_MODULES([SUIL], [suil-0 >= 0.10.0])
#PKG_CHECK_MODULES([PANGO], [pango >= 1.0])
#PKG_CHECK_MODULES([PANGOFT2], [pangoft2 >= 1.0])
PKG_CHECK_MODULES([LIBDAZZLE], [libdazzle-1.0 >= 3.20])
PKG_CHECK_MODULES([LIBXML], [libxml-2.0 >= 2.9.4])
PKG_CHECK_MODULES([LIBSMF], [smf >= 1.3])
PKG_CHECK_MODULES([SNDFILE], [sndfile >= 1.0.28])
PKG_CHECK_MODULES([SAMPLERATE], [samplerate >= 0.1.9])
AM_COND_IF([LINUX],
           [PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])])
AM_COND_IF([WINDOWS],
           [PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])])
PKG_CHECK_MODULES([PORTAUDIO], [portaudio-2.0])
PKG_CHECK_MODULES([LIBCYAML], [libcyaml])

#FFMPEG stuff
PKG_CHECK_MODULES(
  [FFMPEG],
  [libavcodec libavutil libavformat],
  [AC_DEFINE([HAVE_FFMPEG],[1],[Use ffmpeg])])


#Checks for header files.
#Checks for typedefs, structures, and compiler characteristics.
#Checks for library functions.
AM_COND_IF([WINDOWS],
           [AC_CHECK_HEADERS([windows.h])])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_FUNC([mlock],
              [AC_DEFINE([HAVE_MLOCK], [1],
              [Define if mlock exists.])])

#AM_COND_IF([LINUX],
           #[LDFLAGS="$LDFLAGS -L/usr/lib"])

# Check for breeze icons
AM_COND_IF([USE_DEFAULT_PREFIX],
           [AC_CHECK_FILE([$ac_default_prefix/share/icons/breeze-dark],,
              [echo "Breeze icons are required" && \
               echo "Please install them using a package manager or manually from here https://github.com/KDE/breeze-icons" && \
               exit 1])],
           [AC_CHECK_FILE([$prefix/share/icons/breeze-dark],,
              [echo "Breeze icons are required" && \
               echo "Please install them using a package manager or manually from here https://github.com/KDE/breeze-icons" && \
               exit 1])])

# Add GSettings support
GLIB_GSETTINGS

# Declare output files
AC_CONFIG_FILES([
  Makefile
])

# Actually output all declared files
AC_OUTPUT
