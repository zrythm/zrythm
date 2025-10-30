# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_structure_arrangement_unit_tests
  arranger_object_list_model_test.cpp
  arranger_object_factory_test.cpp
  arranger_object_owner_test.h
  arranger_object_owner_test.cpp
  arranger_object_test.h
  arranger_object_test.cpp
  audio_region_test.cpp
  audio_source_object_test.cpp
  automation_point_test.cpp
  automation_region_test.cpp
  bounded_object_test.cpp
  chord_object_test.cpp
  chord_region_test.cpp
  colored_object_test.cpp
  editor_settings_test.cpp
  fadeable_object_test.cpp
  loopable_object_test.cpp
  marker_test.cpp
  midi_note_test.cpp
  midi_region_test.cpp
  piano_roll_test.cpp
  region_renderer_test.cpp
  muteable_object_test.cpp
  named_object_test.cpp
  scale_object_test.cpp
  timeline_data_provider_test.cpp
)

set_target_properties(zrythm_structure_arrangement_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_structure_arrangement_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm_arrangement_lib
  zrythm_test_helpers_lib
)

zrythm_discover_tests(zrythm_structure_arrangement_unit_tests)
