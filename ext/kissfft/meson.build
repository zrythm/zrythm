# SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

kissfft_lib = static_library (
  'kissfft',
  'kiss_fft.c',
  'kiss_fftr.c',
  c_args: [
    '-O3',
    extra_optimizations_cflags,
    extra_extra_optimizations_cflags,
    ],
  )
