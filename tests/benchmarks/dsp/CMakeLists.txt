# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(zrythm_dsp_benchmarks
  fork_join_executor_bench.cpp
  graph_scheduler_bench.cpp
  poly_voice_manager_bench.cpp
)

set_target_properties(zrythm_dsp_benchmarks PROPERTIES
  AUTOMOC OFF
)

target_link_libraries(zrythm_dsp_benchmarks PRIVATE
  benchmark::benchmark_main
  GTest::gmock
  zrythm_dsp_lib
)
target_compile_definitions(zrythm_dsp_benchmarks PRIVATE
  TEST_WAV_FILE_PATH="${CMAKE_SOURCE_DIR}/tests/test.wav"
)
