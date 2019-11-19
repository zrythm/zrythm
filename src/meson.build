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
  ])

if os_darwin
  zrythm_srcs += 'Wrapper.m'
endif

subdir('actions')
subdir('audio')
subdir('gui')
subdir('plugins')
subdir('settings')
subdir('utils')

zrythm_lib = static_library (
  'zrythm-lib',
  sources: [
    zrythm_srcs,
    ext_srcs,
    ],
  dependencies: zrythm_deps,
  include_directories: all_inc,
  c_args: [ common_cflags, strict_cflags ],
  pic: true,
  link_with: [
    cyaml,
    lv2_lib,
    widgets_lib,
    audio_decoder,
    midilib,
    ]
  )

objects = []

zrythm_exe = executable (
  'zrythm',
  sources: [
    zrythm_main,
    zrythm_resources,
    ],
  dependencies : [
    zrythm_deps,
    ],
  link_with: [
    zrythm_lib,
    ],
  export_dynamic: true,
  gui_app: true,
  include_directories: all_inc,
  c_args: [ common_cflags ],
  link_args: common_ldflags,
  objects: objects,
  install: true,
  )
