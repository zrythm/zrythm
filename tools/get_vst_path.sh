#! /bin/sh
# SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This program prints the absolute path of a given vst or
# vst3 plugin basename

set -e

plugin_filename=$1
version=$2

vst_path_env_name="VST${version}_PATH"
vst_path="${!vst_path_env_name}"

if [ -z "$vst_path" ]; then
  vst_path="/usr/lib/vst:/usr/lib/vst3"
fi

for i in $(echo "$vst_path" | sed "s/:/ /g"); do
  full_path="$i/$plugin_filename"
  if ls "$full_path" >/dev/null 2>&1; then
    echo "$full_path";
    exit 0
  fi
done

exit 1
