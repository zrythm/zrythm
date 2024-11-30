# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_dsp_lib STATIC)

target_sources(zrythm_dsp_lib
  PRIVATE
  channel.h
  panning.h
  panning.cpp
  plugin_identifier.h
  plugin_identifier.cpp
  port_identifier.h
  port_identifier.cpp
  position.h
  position.cpp
)

target_precompile_headers(zrythm_dsp_lib REUSE_FROM zrythm_utils_lib)

target_link_libraries(zrythm_dsp_lib PUBLIC ${zrythm_link_libs} zrythm_utils_lib)
target_include_directories(zrythm_dsp_lib PUBLIC ${zrythm_include_dirs})
target_compile_definitions(zrythm_dsp_lib PUBLIC ${zrythm_definitions})
target_compile_options(zrythm_dsp_lib PUBLIC ${zrythm_compile_args})
