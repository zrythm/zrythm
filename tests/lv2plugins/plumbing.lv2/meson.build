# SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
