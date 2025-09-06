# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_actions_unit_tests
  arranger_object_creator_test.cpp
  arranger_object_selection_operator_test.cpp
  mock_undo_stack.h
  parameter_operator_test.cpp
  track_creator_test.cpp
  track_operator_test.cpp
)

set_target_properties(zrythm_actions_unit_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_actions_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_actions_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_actions_unit_tests)
