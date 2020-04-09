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
  join_paths ('audio', 'midi_note.c'),
  join_paths ('audio', 'midi_region.c'),
  join_paths ('audio', 'position.c'),
  join_paths ('audio', 'track.c'),
  join_paths ('audio', 'tracklist.c'),
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

if get_option ('enable_guile')
  if guile_snarf.found()
# .x files
    guile_snarfed_srcs = []
    foreach snarfable_src : guile_snarfable_srcs
      split_by_dot = snarfable_src.split('.')
      without_ext = ''
      foreach sp : split_by_dot
        if sp == split_by_dot[split_by_dot.length() - 2]
          without_ext += sp
        elif sp != split_by_dot[split_by_dot.length() - 1]
          without_ext += sp + '.'
        endif
      endforeach
      new_file = without_ext.split (
        meson.current_source_dir () + '/')[1]
      new_file = new_file.underscorify() + '.x'
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
endif
