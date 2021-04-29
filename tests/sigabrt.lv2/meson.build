# Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

sigabrt_lv2_cdata = configuration_data ()
if os_windows
  sigabrt_lv2_cdata.set ('LIB_EXT', '.dll')
elif os_darwin
  sigabrt_lv2_cdata.set ('LIB_EXT', '.dylib')
else
  sigabrt_lv2_cdata.set ('LIB_EXT', '.so')
endif
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: sigabrt_lv2_cdata,
  )
amp_ttl = configure_file (
  input: 'sigabrt.ttl',
  output: 'sigabrt.ttl',
  configuration: sigabrt_lv2_cdata,
  )

sigabrt_lv2_so = shared_library (
  'sigabrt',
  name_prefix: '',
  sources: [
    'sigabrt.c',
    ],
  dependencies: [ lv2_dep, libm ],
  install: false,
  )

test_lv2_plugin_libs += sigabrt_lv2_so
test_lv2_plugins += {
  'name': 'sigabrt',
  'uri': 'https://www.zrythm.org/plugins/sigabrt',
  'bundle': meson.current_build_dir (),
  'lib': sigabrt_lv2_so,
  }
