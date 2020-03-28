# Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

if get_option('enable_guile')
guile_srcs = files ([
  'guile.c',
  join_paths ('audio', 'position.c'),
  'zrythm.c',
  ])

guilelib = static_library (
  'guilelib',
  guile_srcs,
  dependencies: [ guile_dep, zrythm_deps ],
  c_args: [ common_cflags ],
  include_directories: all_inc,
  )
endif
