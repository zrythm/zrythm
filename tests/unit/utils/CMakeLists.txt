# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_utils_unit_tests
  algorithms_test.cpp
  audio_file_test.cpp
  audio_file_writer_test.cpp
  audio_test.cpp
  compression_test.cpp
  concurrency_test.cpp
  datetime_test.cpp
  debouncer_test.cpp
  directory_manager_test.cpp
  dsp_test.cpp
  expandable_tick_range_test.cpp
  hash_test.cpp
  icloneable_test.cpp
  io_test.cpp
  serialization_test.cpp
  math_test.cpp
  midi_test.cpp
  monotonic_time_provider_test.cpp
  mpmc_queue_test.cpp
  networking_test.cpp
  object_pool_test.cpp
  playback_cache_scheduler_test.cpp
  qt_test.cpp
  ranges_test.cpp
  ring_buffer_test.cpp
  string_test.cpp
  units_test.cpp
  uuid_identifiable_object_test.h
  uuid_identifiable_object_test.cpp
  variant_helpers_test.cpp
  views_test.cpp
)

set_target_properties(zrythm_utils_unit_tests PROPERTIES
  AUTOMOC ON
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_link_libraries(zrythm_utils_unit_tests PRIVATE
  zrythm::gtest_for_tests
  zrythm::utils_lib
  httplib::httplib
  zrythm_test_helpers_lib
)
target_compile_definitions(zrythm_utils_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

zrythm_discover_tests(zrythm_utils_unit_tests)
