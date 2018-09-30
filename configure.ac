# Initialize autoconf: package name, version, bug report address
AC_INIT([zrythm], [0.1], [contact@alextee.website])

# initialize automake. turn on all automake warnings and report them as errors
# this is a foreign package
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

# Check for a C compiler
AC_PROG_CC([gcc])

#Checks for libraries.
#Checks for header files.
#Checks for typedefs, structures, and compiler characteristics.
#Checks for library functions.
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.20])
PKG_CHECK_MODULES([JACK], [jack >= 1.8.17])
PKG_CHECK_MODULES([LV2], [lv2 >= 1.15.3])
PKG_CHECK_MODULES([LILV], [lilv-0 >= 0.24.2])
PKG_CHECK_MODULES([SUIL], [suil-0 >= 0.10.0])

# Declare config.h as output header
AC_CONFIG_HEADERS([config.h])

# Declare Makefile and src/Makefile as output files
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  data/Makefile
])

# Set default prefix
AC_PREFIX_PROGRAM(gcc)

LDFLAGS="$LDFLAGS -L/usr/lib"

# Add GSettings support
GLIB_GSETTINGS

# Actually output all declared files
AC_OUTPUT
