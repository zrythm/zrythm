# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(dsp_unit_tests
  position_test.cpp
)

set_target_properties(dsp_unit_tests PROPERTIES
  AUTOMOC OFF
)

target_link_libraries(dsp_unit_tests PRIVATE
  GTest::gtest
  GTest::gtest_main
  zrythm_dsp_lib
)
target_compile_definitions(dsp_unit_tests PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)

include(GoogleTest)
gtest_discover_tests(dsp_unit_tests)
