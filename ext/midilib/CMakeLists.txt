# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
  set(ENDIANNESS_DEF "HAVE_BIG_ENDIAN=1")
else()
  set(ENDIANNESS_DEF "HAVE_LITTLE_ENDIAN=1")
endif()

add_library(midilib STATIC
  src/midifile.c
  src/midiutil.c
)

target_compile_definitions(midilib INTERFACE ${ENDIANNESS_DEF})
add_library(midilib::midilib ALIAS midilib)