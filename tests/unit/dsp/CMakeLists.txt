# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(dsp_unit_tests
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
  plugin_identifier_test.cpp
  port_identifier_test.cpp
  position_test.cpp
  stretcher_test.cpp
  true_peak_dsp_test.cpp
)

set_target_properties(dsp_unit_tests PROPERTIES
  AUTOMOC OFF
)

target_link_libraries(dsp_unit_tests PRIVATE
  GTest::gtest_main
  GTest::gmock_main
  zrythm_dsp_lib
)
target_compile_definitions(dsp_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

include(GoogleTest)
gtest_discover_tests(dsp_unit_tests)
