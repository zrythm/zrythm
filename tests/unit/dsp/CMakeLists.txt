# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_dsp_unit_tests
  chord_descriptor_test.cpp
  curve_test.cpp
  ditherer_test.cpp
  kmeter_dsp_test.cpp
  graph_builder_test.cpp
  graph_node_test.cpp
  graph_scheduler_test.cpp
  graph_test.cpp
  musical_scale_test.cpp
  panning_test.cpp
  peak_dsp_test.cpp
  peak_fall_smooth_test.cpp
  plugin_slot_test.cpp
  port_identifier_test.cpp
  position_test.cpp
  stretcher_test.cpp
  true_peak_dsp_test.cpp
)

set_target_properties(zrythm_dsp_unit_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ON
)

target_link_libraries(zrythm_dsp_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_dsp_lib
)
target_compile_definitions(zrythm_dsp_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

zrythm_discover_tests(zrythm_dsp_unit_tests)
