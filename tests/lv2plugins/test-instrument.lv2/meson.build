# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
