# SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file(manifest.ttl ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl COPYONLY)
configure_file(test2.ttl ${CMAKE_CURRENT_BINARY_DIR}/test2.ttl COPYONLY)

if(RUBBERBAND_EXECUTABLE)
  set(target_file "${CMAKE_CURRENT_BINARY_DIR}/SLSE Crash Cymbal - 010.wav")
  add_custom_command(
    OUTPUT "${target_file}"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${TEST_WAV2}" "${target_file}"
    DEPENDS "${TEST_WAV2}"
  )
endif()
