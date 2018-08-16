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
PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([DAZZLE], [libdazzle-1.0])
PKG_CHECK_MODULES([JACK], [jack])
PKG_CHECK_MODULES([LV2], [lv2])
PKG_CHECK_MODULES([LILV], [lilv-0])

# Declare config.h as output header
AC_CONFIG_HEADERS([config.h])

# Declare Makefile and src/Makefile as output files
AC_CONFIG_FILES([Makefile src/Makefile])

# Set default prefix
AC_PREFIX_PROGRAM(gcc)

# Actually output all declared files
AC_OUTPUT
