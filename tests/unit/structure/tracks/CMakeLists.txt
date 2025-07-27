# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_structure_tracks_unit_tests
  automation_track_test.cpp
  automation_tracklist_test.cpp
  channel_send_test.cpp
  track_processor_test.cpp
)

set_target_properties(zrythm_structure_tracks_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_structure_tracks_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_structure_tracks_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_structure_tracks_unit_tests)
