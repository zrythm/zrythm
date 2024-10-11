# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    cached_plugin_descriptors.cpp
    cached_plugin_descriptors.h
    carla_discovery.cpp
    carla_discovery.h
    carla_native_plugin.cpp
    carla_native_plugin.h
    collections.cpp
    collections.h
    plugin.cpp
    plugin.h
    plugin_descriptor.cpp
    plugin_descriptor.h
    plugin_identifier.cpp
    plugin_identifier.h
    plugin_manager.cpp
    plugin_manager.h
    plugin_protocol_paths.cpp
    plugin_protocol_paths.h
    plugin_scanner.cpp
    plugin_scanner.h
    protocol.cpp
    protocol.h
    )

add_executable(plugin-scanner
  plugin_scanner_subprocess.h
  plugin_scanner_subprocess.cpp
)
target_link_libraries(plugin-scanner
  juce_lib
  juce_clap_hosting_lib
)
target_include_directories(plugin-scanner
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)
install(TARGETS plugin-scanner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})