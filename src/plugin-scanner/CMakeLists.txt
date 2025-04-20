# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_executable(plugin-scanner
  plugin_scanner_subprocess.h
  plugin_scanner_subprocess.cpp
)
target_link_libraries(plugin-scanner
  juce_lib
)
if(ZRYTHM_WITH_JUCE_CLAP_HOSTING)
  target_link_libraries(plugin-scanner juce_clap_hosting_lib)
endif()
target_include_directories(plugin-scanner
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)
install(TARGETS plugin-scanner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
