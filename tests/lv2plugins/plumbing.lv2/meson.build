# Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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

plumbing_cdata = configuration_data ()
if os_windows
  plumbing_cdata.set ('LIB_EXT', '.dll')
else
  plumbing_cdata.set ('LIB_EXT', '.so')
endif
manifest_ttl = configure_file (
  input: 'manifest.ttl',
  output: 'manifest.ttl',
  configuration: plumbing_cdata,
  )
amp_ttl = configure_file (
  input: 'plumbing.ttl',
  output: 'plumbing.ttl',
  configuration: plumbing_cdata,
  )

plumbing_lv2 = shared_library (
  'plumbing',
  name_prefix: '',
  sources: [
    'midieat.c',
    'plumbing.c',
    'route.c',
    ],
  dependencies: [ lv2_dep, libm ],
  install: false,
  )

test_lv2_plugin_libs += plumbing_lv2
test_lv2_plugins += {
  'name': 'plumbing',
  'uri': 'http://lv2plug.in/plugins/plumbing',
  'bundle': meson.current_build_dir (),
  'lib': plumbing_lv2,
  }
