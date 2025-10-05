# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_structure_scenes_unit_tests
  clip_launcher_test.cpp
  clip_playback_service_test.cpp
  clip_slot_test.cpp
  scene_test.cpp
)

set_target_properties(zrythm_structure_scenes_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_structure_scenes_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_structure_scenes_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_structure_scenes_unit_tests)
