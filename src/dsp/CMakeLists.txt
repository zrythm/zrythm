# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_dsp_lib STATIC)

target_sources(zrythm_dsp_lib
  PRIVATE
    atomic_position.cpp
    atomic_position_qml_adapter.cpp
    audio_pool.cpp
    audio_port.cpp
    chord_descriptor.cpp
    curve.cpp
    cv_port.cpp
    ditherer.cpp
    fader.cpp
    file_audio_source.cpp
    graph.cpp
    graph_builder.cpp
    graph_export.cpp
    graph_node.cpp
    graph_scheduler.cpp
    graph_thread.cpp
    kmeter_dsp.cpp
    metronome.cpp
    midi_event.cpp
    midi_port.cpp
    modulator_macro_processor.cpp
    musical_scale.cpp
    panning.cpp
    parameter.cpp
    passthrough_processors.cpp
    peak_dsp.cpp
    peak_fall_smooth.cpp
    playhead_qml_adapter.cpp
    port.cpp
    port_connection.cpp
    port_connections_manager.cpp
    processor_base.cpp
    snap_grid.cpp
    stretcher.cpp
    tempo_map_qml_adapter.cpp
    timeline_data_cache.cpp
    transport.cpp
    true_peak_dsp.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      atomic_position.h
      atomic_position_qml_adapter.h
      audio_pool.h
      audio_port.h
      chord_descriptor.h
      curve.h
      cv_port.h
      ditherer.h
      dsp.h
      fader.h
      file_audio_source.h
      graph.h
      graph_builder.h
      graph_export.h
      graph_node.h
      graph_scheduler.h
      graph_thread.h
      itransport.h
      kmeter_dsp.h
      metronome.h
      midi_event.h
      midi_panic_processor.h
      midi_port.h
      modulator_macro_processor.h
      musical_scale.h
      parameter.h
      panning.h
      passthrough_processors.h
      peak_dsp.h
      peak_fall_smooth.h
      playhead.h
      playhead_qml_adapter.h
      port.h
      port_all.h
      port_span.h
      port_connection.h
      port_connections_manager.h
      processor_base.h
      snap_grid.h
      stretcher.h
      tempo_map.h
      tempo_map_qml_adapter.h
      timeline_data_cache.h
      transport.h
      true_peak_dsp.h
)

set_target_properties(zrythm_dsp_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
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

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_dsp_lib
  URI ZrythmDsp
  VERSION 1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmDsp
)

# this breaks things on MSVC (:: in path names)
# add_library(zrythm::dsp_lib ALIAS zrythm_dsp_lib)
