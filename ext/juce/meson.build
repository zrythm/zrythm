# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

juce_include_dirs = [
  include_directories('.'),
  include_directories('modules'),
]

juce_cpp_args = [
  '-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1',
  '-DJUCE_DEBUG=' + (get_option('debug') ? '1' : '0'),
  '-fno-single-precision-constant',
  ]
juce_srcs = [
  'modules/juce_core/juce_core.cpp',
  ]
juce_lib_deps = []
if os_darwin
  juce_srcs += ['modules/juce_core/juce_core.mm',]
  juce_lib_deps += dependency (
    'appleframeworks',
    modules: [
      'foundation',
      'cocoa',
      'appkit',
      ])
elif os_windows
  # build fails without this
  juce_cpp_args += [
    '-fpermissive',
    ]
endif

juce_lib = static_library (
  'juce', files(juce_srcs),
  include_directories: juce_include_dirs,
  cpp_args: juce_cpp_args,
  dependencies: juce_lib_deps,
  )

juce_dep = declare_dependency (
  compile_args: [],
  include_directories: juce_include_dirs,
  link_args: [],
  link_with: juce_lib,
  version: '7.0.12',
  )
