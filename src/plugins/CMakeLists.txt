# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense


qt_add_library(zrythm_plugins_lib STATIC)

target_sources(zrythm_plugins_lib
  PRIVATE
    out_of_process_scanner.h
    out_of_process_scanner.cpp
    plugin_configuration.h
    plugin_configuration.cpp
    plugin_descriptor.h
    plugin_descriptor.cpp
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

target_precompile_headers(zrythm_plugins_lib REUSE_FROM zrythm_dsp_lib)

target_link_libraries(zrythm_plugins_lib
  PUBLIC
    zrythm::include_dirs
    ${zrythm_link_libs}
    zrythm::all_compile_options
    zrythm_dsp_lib
)

add_library(zrythm::plugins_lib ALIAS zrythm_plugins_lib)

add_subdirectory(plugin-scanner)
