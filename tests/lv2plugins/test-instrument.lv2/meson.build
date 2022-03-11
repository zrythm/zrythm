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

test_instrument_cdata = configuration_data ()
if os_windows
  test_instrument_cdata.set ('LIB_EXT', '.dll')
else
  test_instrument_cdata.set ('LIB_EXT', '.so')
endif
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: test_instrument_cdata,
  )
test_instrument_ttl = configure_file (
  input: 'test-instrument.ttl',
  output: 'test-instrument.ttl',
  configuration: test_instrument_cdata,
  )

test_instrument_lv2 = shared_library (
  'test-instrument',
  name_prefix: '',
  sources: [
    'test-instrument.c',
    ],
  dependencies: [ lv2_dep, libm ],
  install: false,
  )

test_lv2_plugin_libs += test_instrument_lv2
test_lv2_plugins += {
  'name': 'test-instrument',
  'uri': 'https://lv2.zrythm.org/test-instrument',
  'bundle': meson.current_build_dir (),
  'lib': test_instrument_lv2,
  }
