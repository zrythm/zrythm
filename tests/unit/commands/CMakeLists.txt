# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_commands_unit_tests
  add_arranger_object_command_test.cpp
  add_track_command_test.cpp
  change_parameter_value_command_test.cpp
  change_track_color_command_test.cpp
  move_arranger_objects_command_test.cpp
  move_plugin_command_test.cpp
  relocate_arranger_object_command_test.cpp
  rename_track_command_test.cpp
  route_track_command_test.cpp
)

set_target_properties(zrythm_commands_unit_tests PROPERTIES
  AUTOMOC OFF
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_commands_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_commands_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_commands_unit_tests)
