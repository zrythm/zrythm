# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

zita_resampler_lib = static_library (
  'zita-resampler',
  sources: files ([
    'resampler.cc',
    'resampler-table.cc',
    ]),
  include_directories: [
    include_directories('.'),
    ],
  c_args: [
    common_cflags, '-O3',
    extra_optimizations_cflags,
    extra_extra_optimizations_cflags,
    ],
  pic: true)

zita_resampler_dep = declare_dependency (
  link_with: zita_resampler_lib,
  include_directories: include_directories('.'))