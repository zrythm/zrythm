# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_structure_project_unit_tests
  project_json_builder_test.cpp
)

set_target_properties(zrythm_structure_project_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_structure_project_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_structure_project_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_structure_project_unit_tests)
