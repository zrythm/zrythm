# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of ZPlugins
#
# ZPlugins is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ZPlugins is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with ZPlugins.  If not, see <https://www.gnu.org/licenses/>.

ztoolkit_srcs = [
  'ztk_app.c',
  'ztk_color.c',
  'ztk_knob.c',
  'ztk_label.c',
  'ztk_theme.c',
  'ztk_widget.c',
  ]

_ztoolkit_inc = [
  include_directories ('..'),
  pugl_inc,
  ]

ztoolkit_lib = static_library (
  'Ztoolkit',
  sources: [
    ztoolkit_srcs,
    ],
  dependencies: [
    cairo_dep,
    ],
  include_directories: _ztoolkit_inc,
  c_args: [ common_cflags, strict_cflags, ],
  link_with: [
    pugl,
    ],
  )

