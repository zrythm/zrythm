# SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

weakjack_lib = []
if have_jack
  weakjack_cflags = []
  if use_weak_jack
    weakjack_cflags += [
      '-DUSE_WEAK_JACK=1',
      ]
  endif
  weakjack_lib = static_library (
    'weakjack',
    'weak_libjack.c',
    c_args: weakjack_cflags,
    )
endif
