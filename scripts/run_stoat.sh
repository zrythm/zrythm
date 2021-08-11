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

set -e

# add the following to get graph
# -G stoat_results.png

cd $MESON_BUILD_ROOT
libstoat=$(whereis libstoat.so | awk -F": " '{ print $2 }')
stoat --recursive . -l $libstoat \
  --suppression $MESON_SOURCE_ROOT/tools/stoat_suppressions.txt \
  --whitelist $MESON_SOURCE_ROOT/tools/stoat_whitelist.txt
