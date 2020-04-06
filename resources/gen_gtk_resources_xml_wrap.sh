#!/usr/bin/env sh

cd @MESON_BUILD_ROOT@/resources
@GUILE@ -s gen-gtk-resources-xml.scm $1 $2
