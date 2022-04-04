# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
