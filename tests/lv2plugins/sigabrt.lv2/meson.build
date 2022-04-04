# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
