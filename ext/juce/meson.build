# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

juce_include_dirs = [
  include_directories('.'),
  include_directories('modules'),
]

juce_srcs = [
  'modules/juce_core/juce_core.cpp',
  ]
if os_darwin
  juce_srcs += ['modules/juce_core/juce_core.mm',]
endif

juce_lib = static_library (
  'juce', files(juce_srcs),
  include_directories: juce_include_dirs,
  cpp_args: [
    '-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1',
    '-DJUCE_DEBUG=' + (get_option('debug') ? '1' : '0'),
    '-fno-single-precision-constant',
    ],
  )

juce_dep = declare_dependency (
  compile_args: [],
  include_directories: juce_include_dirs,
  link_args: [],
  link_with: juce_lib,
  version: '7.0.12',
  )
