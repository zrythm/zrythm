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

# This should be call from the zrythm root dir

set -e

export NO_SCAN_PLUGINS=1
export ZRYTHM_DSP_THREADS=1
valgrind --num-callers=160 --leak-check=full \
    --show-leak-kinds=all --track-origins=yes \
    --verbose --log-file=valog \
    --suppressions=tools/vg.sup build/src/zrythm
