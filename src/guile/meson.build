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

_guile_snarfable_srcs = [
  join_paths ('audio', 'position.c'),
  'zrythm.c',
  ]
guile_snarfable_srcs = []
foreach srcfile : _guile_snarfable_srcs
  guile_snarfable_srcs += meson.current_source_dir() / srcfile
endforeach

guile_srcs = [
  'guile.c',
  guile_snarfable_srcs,
  ]

if guile_snarf.found()
# .x files
  guile_snarfed_srcs = []
  foreach snarfable_src : guile_snarfable_srcs
    new_file = snarfable_src.split('.')[0].underscorify() + '.x'
    guile_snarfed_srcs += custom_target (
      new_file,
      output: new_file,
      input: snarfable_src,
      command: [
        guile, '-s', guile_snarf_wrap,
        guile_dep.name(),
        '@OUTPUT@', '@INPUT@', ],
      )
  endforeach

  guile_srcs += guile_snarfed_srcs
endif

guilelib = static_library (
  'guilelib',
  guile_srcs,
  dependencies: [ guile_dep, zrythm_deps ],
  c_args: [ common_cflags ],
  include_directories: all_inc,
  )
