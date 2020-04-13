#!/usr/bin/env sh
# This is a generic wrapper for calling guile scripts
# to work around a bug with guile/mingw/windows

cd @MESON_BUILD_ROOT@
@GUILE@ -s @SCRIPT_NAME@ "$@"
