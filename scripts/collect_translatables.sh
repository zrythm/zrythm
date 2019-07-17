#!/usr/bin/env python3
#
# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

import os
import os.path
import sys

for path in ['inc', 'src', 'resources']:
  for dirpath, dirnames, filenames in os.walk(path):
    for filename in [f for f in filenames if f.endswith(".c") or f.endswith(".h") or f.endswith(".ui")]:
      print(os.path.join(dirpath, filename))
