# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(_crill_lib INTERFACE)

target_include_directories(_crill_lib SYSTEM INTERFACE
  $<BUILD_LOCAL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

add_library(crill::crill ALIAS _crill_lib)
