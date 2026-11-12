# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_operations_unit_tests
  operations_test.cpp
  param_types_test.cpp
)

set_target_properties(zrythm_operations_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_operations_unit_tests PRIVATE
  GTest::gmock_main
  zrythm_operations_lib
  zrythm_test_helpers_lib
  zrythm_test_warning_suppressions_link_last
)

target_precompile_headers(zrythm_operations_unit_tests REUSE_FROM zrythm_actions_unit_tests)

zrythm_discover_tests(zrythm_operations_unit_tests)
