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

cyaml_inc = [
  include_directories ('include'),
  ]
cyaml = static_library (
  'cyaml',
  sources: [
    join_paths ('src', 'mem.c'),
    join_paths ('src', 'save.c'),
    join_paths ('src', 'free.c'),
    join_paths ('src', 'load.c'),
    join_paths ('src', 'utf8.c'),
    join_paths ('src', 'util.c'),
    ],
  dependencies: yaml_dep,
  include_directories: cyaml_inc,
)
