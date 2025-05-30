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
  port_connection.h
  port_connection.cpp
  port_connections_manager.h
  port_connections_manager.cpp
  port_identifier.h
  port_identifier.cpp
  position.h
  position.cpp
  stretcher.h
  stretcher.cpp
  true_peak_dsp.h
  true_peak_dsp.cpp
)

set_target_properties(zrythm_dsp_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_dsp_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dsp.h>)

target_link_libraries(zrythm_dsp_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm::utils_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)

# this breaks things on MSVC (:: in path names)
# add_library(zrythm::dsp_lib ALIAS zrythm_dsp_lib)
