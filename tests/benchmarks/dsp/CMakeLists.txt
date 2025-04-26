# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(dsp_benchmarks
  graph_scheduler_bench.cpp
)

set_target_properties(dsp_benchmarks PROPERTIES
  AUTOMOC OFF
)

target_link_libraries(dsp_benchmarks PRIVATE
  zrythm::benchmark
  zrythm::dsp_lib
)
target_compile_definitions(dsp_benchmarks PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)
