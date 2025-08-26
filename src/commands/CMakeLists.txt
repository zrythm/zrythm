# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_commands_lib STATIC)

target_sources(zrythm_commands_lib
  PRIVATE
    change_parameter_value_command.h
    change_track_color_command.h
    rename_track_command.h
)

set_target_properties(zrythm_commands_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_commands_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/rename_track_command.h>)

target_link_libraries(zrythm_commands_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_structure_tracks_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_commands_lib
  URI ZrythmCommands
  VERSION 1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmCommands
)
