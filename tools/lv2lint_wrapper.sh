#! /bin/sh

#  Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
