# Initialize autoconf: package name, version, bug report address
AC_INIT([zrythm], [0.1], [alex@alextee.website])

# Declare config.h as output header
AC_CONFIG_HEADERS([config.h])

# AC_CANONICAL_HOST is needed to access the 'host_os' variable
AC_CANONICAL_HOST

build_linux=no
build_windows=no
build_mac=no

# Detect the target system
case "${host_os}" in
  linux*)
    build_linux=yes
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
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])
AM_COND_IF([WINDOWS],
           [AC_SUBST([WINDOWS], [1])])
AM_CONDITIONAL([OSX], [test "$build_mac" = "yes"])
AM_COND_IF([OSX],
           [AC_SUBST([OSX], [1])])

# options
AC_ARG_ENABLE(aur-build,
              AS_HELP_STRING([--enable-aur-build], [Used for packaging for AUR]),
              [AC_SUBST([AUR_BUILD], [1])],
              [AC_SUBST([AUR_BUILD], [0])])
AC_ARG_ENABLE(deb-build,
              AS_HELP_STRING([--enable-deb-build], [Used for packaging for Debian]),
              [AC_SUBST([DEB_BUILD], [1])],
              [AC_SUBST([DEB_BUILD], [0])])

AC_ARG_ENABLE(release,
              AS_HELP_STRING([--enable-release], [Build for release (default is debug)]),
              [AC_SUBST([RELEASE], [1])],
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
AM_COND_IF([LINUX],
           [PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])])
AM_COND_IF([WINDOWS],
           [PKG_CHECK_MODULES([LV2], [lv2 >= 1.14.0])])

#Checks for header files.
#Checks for typedefs, structures, and compiler characteristics.
#Checks for library functions.
AM_COND_IF([WINDOWS],
           [AC_CHECK_HEADERS([windows.h])])
AC_CHECK_HEADERS([unistd.h])

AM_COND_IF([LINUX],
           [LDFLAGS="$LDFLAGS -L/usr/lib"])

# Add GSettings support
GLIB_GSETTINGS

# Declare output files
AC_CONFIG_FILES([
  Makefile
])

# Actually output all declared files
AC_OUTPUT
