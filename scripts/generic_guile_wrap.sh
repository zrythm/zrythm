#!/usr/bin/env sh
# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0
#
# This is a generic wrapper for calling guile scripts
# to work around a bug with guile/mingw/windows

cd @SCRIPTS_DIR@
@GUILE@ -s @SCRIPT_NAME@ "$@"
