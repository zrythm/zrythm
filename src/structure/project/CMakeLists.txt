# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Note: this depends on the project schema but the dependency is not kept track
# off anywhere here - until this is addressed, the project must be reconfigured
# manually every time the schema changes.

# Generate header file with embedded project schema as hex byte array
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/file_contents_to_hex_array.py"
    "${CMAKE_SOURCE_DIR}/data/schemas/project.schema.json"
  OUTPUT_VARIABLE SCHEMA_JSON_HEX_BYTES
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# The schema will be embedded as a hex byte array at compile time
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/project_json_schema.in.h"
  "${CMAKE_CURRENT_BINARY_DIR}/project_json_schema.h"
  @ONLY
)

qt_add_library(zrythm_structure_project_lib STATIC)

target_link_libraries(zrythm_structure_project_lib
  PUBLIC
    zrythm_arrangement_lib
    zrythm_structure_tracks_lib
    zrythm_structure_scenes_lib
    zrythm_plugins_lib
    zrythm_dsp_lib
    zrythm::all_compile_options
)

target_sources(zrythm_structure_project_lib
  PRIVATE
    arranger_tool.cpp
    clip_editor.cpp
    project.cpp
    project_graph_builder.cpp
    project_json_serializer.cpp
    project_loader.cpp
    project_path_provider.cpp
    project_saver.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "../.."
    FILES
      arranger_tool.h
      clip_editor.h
      project.h
      project_graph_builder.h
      project_json_serializer.h
      project_loader.h
      project_path_provider.h
      project_saver.h
)

set_target_properties(zrythm_structure_project_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_structure_project_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/project.h>
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_structure_project_lib
  URI ZrythmProject
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_utils_lib
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_structure_tracks_lib
    TARGET zrythm_structure_scenes_lib
    TARGET zrythm_plugins_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmProject
)
