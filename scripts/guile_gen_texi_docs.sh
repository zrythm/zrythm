#!/usr/bin/env bash
#
#  Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
#
#  This file is part of Zrythm
#
#  Zrythm is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Zrythm is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
#
# This file generates .texi documentation from a
#   given C source file with defined SCM_* macros.

set -e

GUILE_SNARF_DOCS_BIN=$1
GUILE_PKGCONF_NAME=$2
INPUT_FILE=$3 # c file
OUTPUT_FILE=$4 # texi file
PRIVATE_DIR=$5 # for intermediate files

input_file_base=$(basename $INPUT_FILE)
input_file_base_noext=$(echo "$input_file_base" | cut -f 1 -d '.')

cd $(dirname $GUILE_SNARF_DOCS_BIN)
mkdir -p "$PRIVATE_DIR"
$GUILE_SNARF_DOCS_BIN -o \
  "$PRIVATE_DIR/$input_file_base_noext.doc" \
  $INPUT_FILE -- \
  $(pkg-config --cflags-only-I $GUILE_PKGCONF_NAME)
cat "$PRIVATE_DIR/$input_file_base_noext.doc" | \
  guild snarf-check-and-output-texi > \
  "$OUTPUT_FILE"
