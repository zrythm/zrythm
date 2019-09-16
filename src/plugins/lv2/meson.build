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

lv2_srcs = [
  'control.c',
  'log.c',
  'lv2_evbuf.c',
  'lv2_gtk.c',
  'qt5_in_gtk3.cpp',
  'state.c',
  'suil_host.c',
  'suil_instance.c',
  'symap.c',
  'urid.c',
  'win_in_gtk3.cpp',
  'worker.c',
  'x11_in_gtk3.c',
  ]

lv2_lib = static_library (
  'lv2-lib',
  sources: lv2_srcs,
  dependencies: zrythm_deps,
  include_directories: all_inc,
  c_args: [ common_cflags ],
  link_with: [
    cyaml,
    ]
  )
