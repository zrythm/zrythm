# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

endianess_flag=''
if host_machine.endian () == 'big'
  endianess_flag = '-DHAVE_BIG_ENDIAN=1'
elif host_machine.endian () == 'little'
  endianess_flag = '-DHAVE_LITTLE_ENDIAN=1'
endif

midilib = static_library (
  'midilib',
  'src/midifile.c',
  'src/midiutil.c',
  c_args: [
    endianess_flag,
    ],
  )
