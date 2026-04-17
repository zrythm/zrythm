# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_integration_tests
  bundled_plugins_scan_test.cpp
  plugin_param_state_test.cpp
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

zrythm_discover_tests(zrythm_integration_tests)
