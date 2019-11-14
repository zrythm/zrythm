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

# plugins require X11
if (x11_dep.found ())
project_uri = 'https://lv2.zrythm.org/plugins'

plugins_root_inc = include_directories ('.')
pugl_inc = include_directories ('pugl')
plugins_common_inc = [
  plugins_root_inc,
  pugl_inc,
  ]

cairo_dep = dependency('cairo', version: '>=1.0.0')
plugin_deps = [
  lv2_dep,
  fftw3_dep,
  x11_dep,
  cairo_dep,
  libm,
]

plugin_cflags = [
  common_cflags, strict_cflags,
  '-fvisibility=hidden',
  ]

subdir ('pugl')
subdir ('ztoolkit')
subdir ('zlfo')
subdir ('tests')
endif
