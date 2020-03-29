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

guile_snarfable_srcs = [
  join_paths ('audio', 'position.c'),
  'zrythm.c',
  ]

deps_for_guile = [
  gtk_dep,
  guile_dep,
  ]

have_lilv = true
if lilv_dep.name() == 'null'
  have_lilv = false
else
  deps_for_guile += lilv_dep
endif

deps_for_guile_str = ''
foreach el : deps_for_guile
  deps_for_guile_str += el.name() + ' '
endforeach
deps_for_guile_str = deps_for_guile_str.strip()

# .x files
guile_snarfed_srcs = []
foreach snarfable_src : guile_snarfable_srcs
  new_file = snarfable_src.split('.')[0] + '.x'
  new_file = '_'.join (new_file.split('/'))
  guile_snarfed_srcs += custom_target (
    new_file,
    output: new_file,
    input: snarfable_src,
    command: [
      guile, '-s', guile_snarf_wrap,
      meson.source_root(),
      deps_for_guile_str, '@0@'.format (have_lilv),
      '@OUTPUT@', '@INPUT@', ],
    )
endforeach

guile_srcs = [
  'guile.c',
  guile_snarfable_srcs,
  guile_snarfed_srcs,
  ]

guilelib = static_library (
  'guilelib',
  guile_srcs,
  dependencies: [ guile_dep, zrythm_deps ],
  c_args: [ common_cflags ],
  include_directories: all_inc,
  )
