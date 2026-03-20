# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_integration_tests
  bundled_vst3_plugins_scan_test.cpp
)

set_target_properties(zrythm_integration_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_integration_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_actions_lib
  zrythm_test_helpers_lib
)

# Pass the VST3 search paths to the test
# Use `:::` to prevent CMake list expansion
string(REPLACE ";" ":::" BUNDLED_VST3_SEARCH_PATHS_STR "${BUNDLED_VST3_SEARCH_PATHS}")
target_compile_definitions(zrythm_integration_tests PRIVATE
  BUNDLED_VST3_SEARCH_PATHS="${BUNDLED_VST3_SEARCH_PATHS_STR}"
  BUNDLED_PLUGINS_COUNT=14
)

# Ensure plugins are built before running tests
add_dependencies(zrythm_integration_tests ${BUNDLED_PLUGINS_FOR_TESTS_TGTS})

zrythm_discover_tests(zrythm_integration_tests)
