# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_gui_qquick_unit_tests
  chord_clip_canvas_test.cpp
  chord_row_list_model_test.cpp
  generic_plugin_ui_controller_test.cpp
  meter_processor_test.cpp
  timeline_position_tracker_test.cpp
  unified_proxy_model_test.cpp
  waveform_peak_computation_test.cpp
)

set_target_properties(zrythm_gui_qquick_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_gui_qquick_unit_tests PRIVATE
  GTest::gmock_main
  zrythm_gui_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_gui_qquick_unit_tests)
