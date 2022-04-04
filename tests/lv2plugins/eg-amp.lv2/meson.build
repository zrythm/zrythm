# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
