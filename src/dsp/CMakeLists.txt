# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_dsp_lib STATIC)

target_sources(zrythm_dsp_lib
  PRIVATE
  channel.h
  chord_descriptor.h
  chord_descriptor.cpp
  curve.h
  curve.cpp
  ditherer.h
  ditherer.cpp
  dsp.h
  graph.h
  graph.cpp
  graph_builder.h
  graph_builder.cpp
  graph_node.h
  graph_node.cpp
  graph_scheduler.h
  graph_scheduler.cpp
  graph_thread.h
  graph_thread.cpp
  itransport.h
  kmeter_dsp.h
  kmeter_dsp.cpp
  musical_scale.h
  musical_scale.cpp
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

target_precompile_headers(zrythm_dsp_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dsp.h>)

target_link_libraries(zrythm_dsp_lib PUBLIC ${zrythm_link_libs} zrythm_utils_lib)
target_include_directories(zrythm_dsp_lib PUBLIC ${zrythm_include_dirs})
target_compile_definitions(zrythm_dsp_lib PUBLIC ${zrythm_definitions})
target_compile_options(zrythm_dsp_lib PUBLIC ${zrythm_compile_args})
