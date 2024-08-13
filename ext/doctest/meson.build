# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

doctest_lib = static_library('doctest',
  ['doctest.h', 'doctest.cpp'],
   cpp_args: ['-DDOCTEST_CONFIG_IMPLEMENTATION_IN_DLL'],
)

doctest_dep = declare_dependency (
  link_with: doctest_lib)