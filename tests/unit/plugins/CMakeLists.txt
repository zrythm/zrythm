# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_plugins_unit_tests
  clap_plugin_test.cpp
  faust_plugin_test.cpp
  faust_synth_voice_test.cpp
  juce_plugin_test.cpp
  plugin_test.cpp
  plugin_descriptor_test.cpp
  plugin_descriptor_list_test.cpp
  plugin_factory_test.cpp
  plugin_group_test.cpp
  plugin_protocol_test.cpp
  plugin_run_loop_test.cpp
  plugin_scan_manager_test.cpp
  vst3_plugin_test.cpp
)

set_target_properties(zrythm_plugins_unit_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_plugins_unit_tests PRIVATE
  GTest::gmock_main
  zrythm_plugins_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_plugins_unit_tests)
