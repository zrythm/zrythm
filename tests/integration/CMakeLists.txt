# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_integration_tests
  test_plugins_scan_test.cpp
  plugin_integration_test.cpp
  port_observer_graph_test.cpp
  project_serialization_roundtrip_test.cpp
  recording_pipeline_test.cpp
)

set_target_properties(zrythm_integration_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_integration_tests PRIVATE
  GTest::gmock_main
  zrythm_actions_lib
  zrythm_controllers_lib
  zrythm_test_helpers_lib
)

# Absolute path to the plugin scanner executable (with the platform's
# executable suffix), for tests that launch it as a subprocess
target_compile_definitions(zrythm_integration_tests PRIVATE
  TEST_PLUGIN_SCANNER_PATH="$<TARGET_FILE:plugin-scanner>"
)
add_dependencies(zrythm_integration_tests plugin-scanner)

target_precompile_headers(zrythm_integration_tests REUSE_FROM zrythm_structure_project_unit_tests)

zrythm_discover_tests(zrythm_integration_tests)
