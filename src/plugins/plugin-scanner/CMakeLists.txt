# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(plugin-scanner
  WIN32 # not a GUI application but build fails otherwise
  plugin_scanner_subprocess.h
  plugin_scanner_subprocess.cpp
)
target_link_libraries(plugin-scanner
  zrythm::juce_libs
  zrythm_plugins_lib
)
set_target_properties(plugin-scanner PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

target_include_directories(plugin-scanner
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)
