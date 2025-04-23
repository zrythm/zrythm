# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(zita-resampler STATIC
  resampler.cc
  resampler-table.cc
)

target_include_directories(zita-resampler PUBLIC SYSTEM
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(zita-resampler PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O3>
)

# set_target_properties(zita-resampler PROPERTIES
#   POSITION_INDEPENDENT_CODE ON
# )

add_library(zita-resampler::zita-resampler ALIAS zita-resampler)