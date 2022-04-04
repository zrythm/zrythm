#! /bin/sh
# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This is used during the tests to create a temporary
# lv2 plugin environment before calling lv2lint

set -e

cd $MESON_BUILD_ROOT

plugin_dir_name=$1
base_plugin_dir_name=$(basename "$plugin_dir_name")
plugin_uri=$2
tmpdir="/tmp/zlv2_test"
rm -rf $tmpdir
mkdir -pv $tmpdir/$base_plugin_dir_name
cp -fv $plugin_dir_name/*.ttl \
  $plugin_dir_name/*.so \
  $tmpdir/$base_plugin_dir_name/
LV2_PATH=$tmpdir lv2lint $plugin_uri
