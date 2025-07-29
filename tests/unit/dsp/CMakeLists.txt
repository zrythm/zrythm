# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_dsp_unit_tests
  atomic_position_qml_adapter_test.cpp
  atomic_position_test.cpp
  audio_pool_test.cpp
  audio_port_test.cpp
  chord_descriptor_test.cpp
  curve_test.cpp
  cv_port_test.cpp
  ditherer_test.cpp
  file_audio_source_test.cpp
  kmeter_dsp_test.cpp
  graph_builder_test.cpp
  graph_export_test.cpp
  graph_helpers.h
  graph_node_test.cpp
  graph_scheduler_test.cpp
  graph_test.cpp
  midi_event_test.cpp
  midi_panic_processor_test.cpp
  midi_port_test.cpp
  modulator_macro_processor_test.cpp
  musical_scale_test.cpp
  parameter_test.cpp
  panning_test.cpp
  passthrough_processors_test.cpp
  peak_dsp_test.cpp
  peak_fall_smooth_test.cpp
  playhead_test.cpp
  playhead_qml_adapter_test.cpp
  port_connection_test.cpp
  port_connections_manager_test.cpp
  port_span_test.cpp
  port_test.cpp
  position_test.cpp
  processor_base_test.cpp
  stretcher_test.cpp
  tempo_map_test.cpp
  tempo_map_qml_adapter_test.cpp
  true_peak_dsp_test.cpp
)

set_target_properties(zrythm_dsp_unit_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_dsp_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_dsp_lib
  zrythm_test_helpers_lib
)
target_compile_definitions(zrythm_dsp_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

zrythm_discover_tests(zrythm_dsp_unit_tests)
