#! /bin/sh
# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This is used during the tests to create a temporary
# lv2 plugin environment before calling lv2lint

set -e

plugin_filename=$1

for i in $(echo $VST_PATH | sed "s/:/ /g"); do
  full_path="$i/$plugin_filename"
  if ls $full_path; then
    exit 0
  fi
done

exit 1
