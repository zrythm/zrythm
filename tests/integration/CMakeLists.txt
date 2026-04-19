# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_integration_tests
  bundled_plugins_scan_test.cpp
  plugin_integration_test.cpp
  project_serialization_roundtrip_test.cpp
)

set_target_properties(zrythm_integration_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_integration_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_actions_lib
  zrythm_controllers_lib
  zrythm_test_helpers_lib
)

target_precompile_headers(zrythm_integration_tests REUSE_FROM zrythm_structure_project_unit_tests)

zrythm_discover_tests(zrythm_integration_tests)
