# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_undo_unit_tests
  undo_stack_test.cpp
)

set_target_properties(zrythm_undo_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_undo_unit_tests PRIVATE
  GTest::gmock_main
  zrythm_undo_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_undo_unit_tests)
