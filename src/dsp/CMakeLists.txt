# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_dsp_lib STATIC)

target_sources(zrythm_dsp_lib
  PRIVATE
  channel.h
  kmeter_dsp.h
  kmeter_dsp.cpp
  panning.h
  panning.cpp
  peak_dsp.h
  peak_dsp.cpp
  peak_fall_smooth.h
  peak_fall_smooth.cpp
  plugin_identifier.h
  plugin_identifier.cpp
  port_identifier.h
  port_identifier.cpp
  position.h
  position.cpp
  stretcher.h
  stretcher.cpp
  true_peak_dsp.h
  true_peak_dsp.cpp
)

target_precompile_headers(zrythm_dsp_lib REUSE_FROM zrythm_utils_lib)

target_link_libraries(zrythm_dsp_lib PUBLIC ${zrythm_link_libs} zrythm_utils_lib)
target_include_directories(zrythm_dsp_lib PUBLIC ${zrythm_include_dirs})
target_compile_definitions(zrythm_dsp_lib PUBLIC ${zrythm_definitions})
target_compile_options(zrythm_dsp_lib PUBLIC ${zrythm_compile_args})
