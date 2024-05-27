# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

juce_include_dirs = [
  include_directories('.'),
  include_directories('modules'),
]

juce_srcs = [
  'modules/juce_core/juce_core.cpp',
  ]

juce_cpp_public_args = [
  '-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1',
  '-DJUCE_DEBUG=' + (get_option('optimization') == '0' ? '1' : '0'),
  '-DDEBUG=' + (get_option('optimization') == '0' ? '1' : '0'),
  ]

juce_cpp_private_args = [
  '-fno-single-precision-constant',
  ]
juce_lib_deps = []
if os_darwin
  juce_cpp_private_args += [
     '-ObjC++',
    ]
  juce_lib_deps += dependency (
    'appleframeworks',
    modules: [
      'foundation',
      'cocoa',
      'appkit',
      'iokit',
      'security',
      ])
elif os_windows
  # build fails without this
  juce_cpp_private_args += [
    '-fpermissive',
    ]
  juce_lib_deps += [
    cc.find_library('comdlg32'),
    cc.find_library('gdi32'),
    cc.find_library('imm32'),
    cc.find_library('ole32'),
    cc.find_library('oleaut32'),
    cc.find_library('shlwapi'),
    cc.find_library('uuid'),
    cc.find_library('version'),
    cc.find_library('wininet'),
    cc.find_library('winmm'),
    cc.find_library('ws2_32'),
    ]
endif

juce_lib = static_library (
  'juce', files(juce_srcs),
  include_directories: juce_include_dirs,
  cpp_args: [juce_cpp_public_args, juce_cpp_private_args],
  dependencies: juce_lib_deps,
  )

juce_dep = declare_dependency (
  compile_args: juce_cpp_public_args,
  include_directories: juce_include_dirs,
  link_args: [],
  link_with: juce_lib,
  version: '7.0.12',
  )
