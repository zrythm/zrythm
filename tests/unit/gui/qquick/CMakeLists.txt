# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_gui_qquick_unit_tests
  chord_clip_segmenter_test.cpp
  chord_row_list_model_test.cpp
  timeline_position_tracker_test.cpp
)

set_target_properties(zrythm_gui_qquick_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_gui_qquick_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_gui_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_gui_qquick_unit_tests)
