#!/usr/bin/env sh

cd @MESON_BUILD_ROOT@
@GUILE@ -s meson-post-install.scm
