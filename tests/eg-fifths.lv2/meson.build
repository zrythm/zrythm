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

eg_fifths_cdata = configuration_data ()
if os_windows
  eg_fifths_cdata.set ('LIB_EXT', '.dll')
else
  eg_fifths_cdata.set ('LIB_EXT', '.so')
endif
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: eg_fifths_cdata,
  )
fifths_ttl = configure_file (
  input: 'fifths.ttl',
  output: 'fifths.ttl',
  configuration: eg_fifths_cdata,
  )

eg_fifths_lv2 = shared_library (
  'fifths',
  name_prefix: '',
  sources: [
    'fifths.c',
    ],
  dependencies: [ lv2_dep, libm ],
  install: false,
  )
