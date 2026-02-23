# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_controllers_unit_tests
  project_json_serializer_roundtrip_test.cpp
  project_json_serializer_structure_test.cpp
  project_json_serializer_validation_test.cpp
  project_loader_test.cpp
  project_saver_test.cpp
  transport_controller_test.cpp
)

set_target_properties(zrythm_controllers_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_controllers_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_controllers_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_controllers_unit_tests)
