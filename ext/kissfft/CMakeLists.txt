# SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(kissfft STATIC
  kiss_fft.c
  kiss_fftr.c
)
target_include_directories(kissfft
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(kissfft PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -O3
    # recommended by kissfft's TIPS.md
    -fomit-frame-pointer
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /O2
    # MSVC equivalent of -fomit-frame-pointer
    /Oy
  >
)

add_library(kissfft::kissfft ALIAS kissfft)