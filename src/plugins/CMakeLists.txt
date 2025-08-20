# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_plugins_lib STATIC)

target_sources(zrythm_plugins_lib
  PRIVATE
    # carla_native_plugin.h
    # carla_native_plugin.cpp
    clap_plugin.h
    clap_plugin.cpp
    clap_plugin_param.h
    clap_plugin_param.cpp
    internal_plugin_base.h
    internal_plugin_base.cpp
    juce_plugin.h
    juce_plugin.cpp
    out_of_process_scanner.h
    out_of_process_scanner.cpp
    plugin.h
    plugin.cpp
    plugin_all.h
    plugin_configuration.h
    plugin_configuration.cpp
    plugin_descriptor.h
    plugin_descriptor.cpp
    plugin_list.h
    plugin_protocol.h
    plugin_protocol.cpp
    plugin_slot.h
    plugin_slot.cpp
    plugin_descriptor_list.h
    plugin_descriptor_list.cpp
    plugin_scan_manager.h
    plugin_scan_manager.cpp
)

set_target_properties(zrythm_plugins_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_plugins_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/plugin_list.h>
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/plugin_scan_manager.h>
)

target_link_libraries(zrythm_plugins_lib
  PUBLIC
    zrythm::include_dirs
    ${zrythm_link_libs}
    zrythm::all_compile_options
    zrythm_dsp_lib
)

add_library(zrythm::plugins_lib ALIAS zrythm_plugins_lib)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_plugins_lib
  URI ZrythmPlugins
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_dsp_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmPlugins
)

add_subdirectory(plugin-scanner)
# add_subdirectory(lv2apply)
