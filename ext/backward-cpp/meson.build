# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

backward_cpp_inc = include_directories('.')

backward_cpp_args = []
if os_gnu
  backward_cpp_args += [
     '-DBACKWARD_HAS_DW=1', '-DBACKWARD_HAS_UNWIND=1']
elif os_darwin
  backward_cpp_args += ['-DBACKWARD_HAS_UNWIND=1']
endif

backward_cpp_deps = [ dependency('threads') ]
if os_gnu
  backward_cpp_deps += dependency('libdw')
elif os_windows
  backward_cpp_deps += dbg_help_lib
endif

backward_cpp_dep = declare_dependency(
  include_directories : backward_cpp_inc,
  dependencies: backward_cpp_deps,
  compile_args : backward_cpp_args,
)
