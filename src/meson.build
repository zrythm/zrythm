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

zrythm_main = files ('main.c')
zrythm_srcs = files ([
  'project.c',
  'zrythm.c',
  'zrythm_app.c',
  ])

if os_darwin
  zrythm_srcs += 'Wrapper.m'
endif

# add the translators C string header to the sources
translators_c_h = configure_file (
  input: translators_c_header_gen_wrap,
  output: 'translators.h',
  command: [
    '@INPUT0@', '@OUTPUT@', 'about',
    meson.source_root () / 'TRANSLATORS',
    ],
  )
zrythm_srcs += translators_c_h

subdir('actions')
subdir('audio')
subdir('gui')
subdir('guile')
subdir('plugins')
subdir('settings')
subdir('utils')

zrythm_link_libs = [
  lv2_lib,
  widgets_lib,
  midilib,
  zita_resampler_lib,
  ]

if guile_dep.found ()
  zrythm_link_libs += guilelib
endif

# there are problems if added to zrythm_srcs directly
zrythm_lib_srcs = zrythm_srcs
if os_windows
  zrythm_lib_srcs += win_res
endif

# static is used in the executable and shared is
# used in tests
zrythm_lib = both_libraries (
  'zrythm-lib',
  sources: [
    zrythm_lib_srcs,
    ext_srcs,
    ],
  dependencies: zrythm_deps,
  include_directories: all_inc,
  c_args: [ common_cflags, strict_cflags ],
  pic: true,
  link_with: zrythm_link_libs,
  )

objects = []

zrythm_exe = executable (
  prog_name_lowercase,
  sources: [
    zrythm_main,
    zrythm_resources,
    ],
  dependencies : [
    zrythm_deps,
    ],
  link_with: [
    zrythm_lib.get_static_lib (),
    ],
  export_dynamic: true,
  gui_app: true,
  include_directories: all_inc,
  c_args: [ common_cflags ],
  link_args: common_ldflags,
  objects: objects,
  install: true,
  )

# run stoat
if (stoat.found() and
  meson.get_compiler('c').get_id() == 'clang' and
  meson.get_compiler('cpp').get_id() == 'clang')
  run_target (
    'run_stoat',
    command: [ meson.source_root () / 'scripts' / 'run_stoat.sh', ],
    depends: [ zrythm_lib, ],
    )
endif
