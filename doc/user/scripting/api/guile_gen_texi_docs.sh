#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This file generates .texi documentation from a
#   given C source file with defined SCM_* macros.

set -e

GUILE_SNARF_DOCS_BIN=$1
GUILE_PKGCONF_NAME=$2
INPUT_FILE=$3 # c file
OUTPUT_FILE=$4 # texi file
PRIVATE_DIR=$5 # for intermediate files
GUILD_BIN=$6
OTHER_INCLUDES=$7

input_file_base=$(basename $INPUT_FILE)
input_file_base_noext=$(echo "$input_file_base" | cut -f 1 -d '.')

# change to abs paths
output_file="`pwd`/$OUTPUT_FILE"
private_dir="`pwd`/$PRIVATE_DIR"

pushd $(dirname $GUILE_SNARF_DOCS_BIN)
mkdir -p "$private_dir"
# snarf docs out of the C file into a doc file
echo "Snarfing docs out of <$INPUT_FILE> to <$private_dir/$input_file_base_noext.doc>"
$GUILE_SNARF_DOCS_BIN -o \
  "$private_dir/$input_file_base_noext.doc" \
  $INPUT_FILE -- \
  "$OTHER_INCLUDES" \
  $(pkg-config --cflags-only-I $GUILE_PKGCONF_NAME)
# convert to texi
echo "Converting <$private_dir/$input_file_base_noext.doc> to <$output_file>"
cat "$private_dir/$input_file_base_noext.doc" | \
  $GUILD_BIN snarf-check-and-output-texi > \
  "$output_file"
popd
