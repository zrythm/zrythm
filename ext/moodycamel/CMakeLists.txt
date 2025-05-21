# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(_lightweight_semaphore INTERFACE)

target_include_directories(_lightweight_semaphore INTERFACE SYSTEM
  $<BUILD_LOCAL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

add_library(zrythm::lightweight_semaphore ALIAS _lightweight_semaphore)
