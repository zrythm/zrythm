#! /bin/sh
# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set -e

# add the following to get graph
# -G stoat_results.png

cd $MESON_BUILD_ROOT
libstoat=$(whereis libstoat.so | awk -F": " '{ print $2 }')
stoat --recursive . -l $libstoat \
  --suppression $MESON_SOURCE_ROOT/tools/stoat_suppressions.txt \
  --whitelist $MESON_SOURCE_ROOT/tools/stoat_whitelist.txt
