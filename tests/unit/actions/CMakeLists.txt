# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_actions_unit_tests
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
