# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_plugins_unit_tests
  plugin_descriptor_test.cpp
  plugin_protocol_test.cpp
  plugin_scan_manager_test.cpp
  plugin_slot_test.cpp
)

set_target_properties(zrythm_plugins_unit_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_plugins_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_plugins_lib
  zrythm_test_helpers_lib
)

target_compile_definitions(zrythm_plugins_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

zrythm_discover_tests(zrythm_plugins_unit_tests)
