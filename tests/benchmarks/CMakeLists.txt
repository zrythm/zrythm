# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_subdirectory(dsp)

add_custom_target(
  run_all_benchmarks
  COMMAND $<TARGET_FILE:zrythm_dsp_benchmarks>
  COMMENT "Running benchmarks..."
  USES_TERMINAL
)
