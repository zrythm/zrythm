# SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# VST3 hosting glue: plugprovider, vstpresetfile and the platform module
# loaders are not part of the SDK's sdk_hosting target - host consumers must
# compile them in (same pattern as the SDK's moduleinfotool sample). Kept in a
# separate static lib so the third-party sources stay out of unity builds /
# precompiled headers.
add_library(zrythm_vst3_hosting_lib STATIC
  ${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
  ${SDK_ROOT}/public.sdk/source/vst/vstpresetfile.cpp
)
if (SMTG_MAC)
  target_sources(zrythm_vst3_hosting_lib PRIVATE
    ${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
  )
  set_source_files_properties(
    ${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
  )
  find_library(COCOA_FRAMEWORK Cocoa)
  target_link_libraries(zrythm_vst3_hosting_lib PRIVATE ${COCOA_FRAMEWORK})
elseif (SMTG_LINUX)
  target_sources(zrythm_vst3_hosting_lib PRIVATE
    ${SDK_ROOT}/public.sdk/source/vst/hosting/module_linux.cpp
  )
elseif (SMTG_WIN)
  target_sources(zrythm_vst3_hosting_lib PRIVATE
    ${SDK_ROOT}/public.sdk/source/vst/hosting/module_win32.cpp
  )
endif ()
if (MSVC)
  # module_win32.cpp relies on __cplusplus reporting the real standard version
  target_compile_options(zrythm_vst3_hosting_lib PRIVATE /Zc:__cplusplus)
endif ()
zrythm_target_disable_warnings(zrythm_vst3_hosting_lib)
target_link_libraries(zrythm_vst3_hosting_lib PUBLIC sdk_hosting)

qt_add_library(zrythm_plugins_lib STATIC)

target_sources(zrythm_plugins_lib
  PRIVATE
    CLAPPluginFormat.cpp
    clap_plugin.cpp
    host_window_units.cpp
    internal_plugin_base.cpp
    juce_plugin.cpp
    lv2_discovery.cpp
    lv2_plugin_format.cpp
    out_of_process_scanner.cpp
    plugin.cpp
    plugin_configuration.cpp
    plugin_descriptor.cpp
    plugin_descriptor_list.cpp
    plugin_group.cpp
    plugin_host_window.cpp
    plugin_library.cpp
    plugin_parameter_list_model.cpp
    plugin_preset_list_model.cpp
    plugin_protocol.cpp
    plugin_run_loop.cpp
    plugin_format_utils.cpp
    plugin_scan_manager.cpp
    vst3_plugin.cpp
    vst3_plugin_format.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      CLAPPluginFormat.h
      clap_plugin.h
      clap_speaker_arrangement.h
      gl_context_utils.h
      host_window_units.h
      internal_plugin_base.h
      juce_plugin.h
      lv2_discovery.h
      lv2_plugin_format.h
      out_of_process_scanner.h
      plugin.h
      plugin_all.h
      plugin_configuration.h
      plugin_descriptor.h
      plugin_descriptor_list.h
      plugin_factory.h
      plugin_format_utils.h
      plugin_group.h
      plugin_host_window.h
      plugin_library.h
      plugin_parameter_list_model.h
      plugin_preset_list_model.h
      plugin_protocol.h
      plugin_run_loop.h
      plugin_scan_manager.h
      plugin_transport_context.h
      vst3_channel_mapping.h
      vst3_speaker_arrangement.h
      vst3_event_validation.h
      vst3_plugin.h
      vst3_plugin_format.h
)

set_target_properties(zrythm_plugins_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_link_libraries(zrythm_plugins_lib PRIVATE lilv::lilv)
# Stage the LV2 spec bundles shipped with the conan lv2 package into the
# build tree, where the runtime looks for them (datadir/zrythm/lv2-specs
# under the binaries' parent directory), and install them to the same
# location for packaging. This must NOT be a directory that is searched
# for plugins (e.g. lib/zrythm/lv2 or share/zrythm/lv2 are search paths
# for bundled plugins like ZLFO).
cmake_path(
  GET CMAKE_RUNTIME_OUTPUT_DIRECTORY PARENT_PATH
  zrythm_lv2_binaries_parent_dir)
add_custom_command(TARGET zrythm_plugins_lib POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${lv2_RES_DIRS}/lv2"
    "${zrythm_lv2_binaries_parent_dir}/${CMAKE_INSTALL_DATADIR}/${prog_name_lowercase}/lv2-specs"
  COMMENT "Staging LV2 spec bundles"
)
install(
  DIRECTORY "${lv2_RES_DIRS}/lv2/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${prog_name_lowercase}/lv2-specs
)

target_precompile_headers(zrythm_plugins_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/plugin_group.h>
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/plugin_scan_manager.h>
)

target_link_libraries(zrythm_plugins_lib
  PUBLIC
    zrythm::include_dirs
    ${zrythm_link_libs}
    zrythm::all_compile_options
    zrythm_dsp_lib
    zrythm_vst3_hosting_lib
    $<$<TARGET_EXISTS:zrythm_faust_plugins>:zrythm_faust_plugins>
)

add_library(zrythm::plugins_lib ALIAS zrythm_plugins_lib)

add_subdirectory(faust)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_plugins_lib
  URI ZrythmPlugins
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_utils_lib
    TARGET zrythm_dsp_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmPlugins
)
set_target_properties(zrythm_plugins_libplugin PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

add_subdirectory(plugin-scanner)
# add_subdirectory(lv2apply)
