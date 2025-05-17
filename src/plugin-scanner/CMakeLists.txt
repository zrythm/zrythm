# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(plugin-scanner
  plugin_scanner_subprocess.h
  plugin_scanner_subprocess.cpp
)
target_link_libraries(plugin-scanner
  zrythm::juce_libs
)
target_include_directories(plugin-scanner
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)