# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

zita_resampler_lib = static_library (
  'zita-resampler',
  sources: files ([
    'resampler.c',
    'resampler_table.c',
    ]),
  c_args: [
    common_cflags, '-O3',
    extra_optimizations_cflags,
    extra_extra_optimizations_cflags,
    ],
  pic: true)
