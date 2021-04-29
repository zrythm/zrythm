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

eg_amp_cdata = configuration_data ()
if os_windows
  eg_amp_cdata.set ('LIB_EXT', '.dll')
else
  eg_amp_cdata.set ('LIB_EXT', '.so')
endif
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: eg_amp_cdata,
  )
amp_ttl = configure_file (
  input: 'amp.ttl',
  output: 'amp.ttl',
  configuration: eg_amp_cdata,
  )

eg_amp_lv2 = shared_library (
  'amp',
  name_prefix: '',
  sources: [
    'amp.c',
    ],
  dependencies: [ lv2_dep, libm ],
  install: false,
  )

test_lv2_plugin_libs += eg_amp_lv2
test_lv2_plugins += {
  'name': 'eg-amp',
  'uri': 'http://lv2plug.in/plugins/eg-amp',
  'bundle': meson.current_build_dir (),
  'lib': eg_amp_lv2,
  }
