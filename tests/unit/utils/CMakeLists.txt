# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(utils_unit_tests
  algorithms_test.cpp
  audio_file_test.cpp
  audio_test.cpp
  compression_test.cpp
  concurrency_test.cpp
  datetime_test.cpp
  directory_manager_test.cpp
  dsp_test.cpp
  hash_test.cpp
  icloneable_test.cpp
  io_test.cpp
  json_test.cpp
  iserializable_test.cpp
  math_test.cpp
  midi_test.cpp
  mpmc_queue_test.cpp
  object_pool_test.cpp
  ring_buffer_test.cpp
  string_test.cpp
  string_array_test.cpp
)

if(ZRYTHM_TESTS_WITH_INTERNET_ACCESS)
  target_sources(utils_unit_tests PRIVATE networking_test.cpp)
endif()

set_target_properties(utils_unit_tests PROPERTIES
  AUTOMOC OFF
)

target_link_libraries(utils_unit_tests PRIVATE
  GTest::gtest
  GTest::gtest_main
  zrythm_utils_lib
)
target_compile_definitions(utils_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

include(GoogleTest)
gtest_discover_tests(utils_unit_tests)
