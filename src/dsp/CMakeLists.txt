# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_dsp_lib STATIC)

target_sources(zrythm_dsp_lib
  PRIVATE
    atomic_position.h
    atomic_position_qml_adapter.h
    atomic_position_qml_adapter.cpp
    audio_pool.h
    audio_pool.cpp
    audio_port.h
    audio_port.cpp
    chord_descriptor.h
    chord_descriptor.cpp
    curve.h
    curve.cpp
    cv_port.h
    cv_port.cpp
    ditherer.h
    ditherer.cpp
    dsp.h
    file_audio_source.h
    file_audio_source.cpp
    graph.h
    graph.cpp
    graph_builder.h
    graph_builder.cpp
    graph_export.h
    graph_export.cpp
    graph_node.h
    graph_node.cpp
    graph_scheduler.h
    graph_scheduler.cpp
    graph_thread.h
    graph_thread.cpp
    itransport.h
    kmeter_dsp.h
    kmeter_dsp.cpp
    metronome.h
    metronome.cpp
    midi_event.h
    midi_event.cpp
    midi_panic_processor.h
    midi_port.h
    midi_port.cpp
    modulator_macro_processor.h
    modulator_macro_processor.cpp
    musical_scale.h
    musical_scale.cpp
    parameter.h
    parameter.cpp
    panning.h
    panning.cpp
    passthrough_processors.h
    peak_dsp.h
    peak_dsp.cpp
    peak_fall_smooth.h
    peak_fall_smooth.cpp
    playhead.h
    playhead_qml_adapter.h
    playhead_qml_adapter.cpp
    port.h
    port.cpp
    port_all.h
    port_span.h
    port_connection.h
    port_connection.cpp
    port_connections_manager.h
    port_connections_manager.cpp
    position.h
    position.cpp
    processor_base.h
    processor_base.cpp
    stretcher.h
    stretcher.cpp
    tempo_map.h
    tempo_map_qml_adapter.h
    tempo_map_qml_adapter.cpp
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

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_dsp_lib
  URI ZrythmDsp
  VERSION 1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmDsp
)

# this breaks things on MSVC (:: in path names)
# add_library(zrythm::dsp_lib ALIAS zrythm_dsp_lib)
