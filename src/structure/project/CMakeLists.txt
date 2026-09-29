# SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# The schemas reference shared definitions from common.schema.json - inline
# them into self-contained schemas first, then embed those as hex byte
# arrays. Generation runs at build time with the schema files as
# dependencies, so editing a schema regenerates the embedded headers on the
# next build without a manual reconfigure.
set(PROJECT_SCHEMA_INLINED_JSON "${CMAKE_CURRENT_BINARY_DIR}/project.schema.inlined.json")
add_custom_command(
  OUTPUT "${PROJECT_SCHEMA_INLINED_JSON}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/inline_json_schema_refs.py"
    "${CMAKE_SOURCE_DIR}/data/schemas/project.schema.json"
    "${PROJECT_SCHEMA_INLINED_JSON}"
  DEPENDS
    "${CMAKE_SOURCE_DIR}/data/schemas/project.schema.json"
    "${CMAKE_SOURCE_DIR}/data/schemas/common.schema.json"
    "${CMAKE_SOURCE_DIR}/scripts/inline_json_schema_refs.py"
  COMMENT "Inlining shared schema definitions into project.schema.json"
)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/project_json_schema.h"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/generate_embedded_header.py"
    "${PROJECT_SCHEMA_INLINED_JSON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/project_json_schema.in.h"
    "${CMAKE_CURRENT_BINARY_DIR}/project_json_schema.h"
    "@SCHEMA_JSON_HEX_BYTES@"
  DEPENDS
    "${PROJECT_SCHEMA_INLINED_JSON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/project_json_schema.in.h"
    "${CMAKE_SOURCE_DIR}/scripts/generate_embedded_header.py"
  COMMENT "Generating embedded project JSON schema header"
)

# Same for the clipboard schema
set(CLIPBOARD_SCHEMA_INLINED_JSON "${CMAKE_CURRENT_BINARY_DIR}/clipboard.schema.inlined.json")
add_custom_command(
  OUTPUT "${CLIPBOARD_SCHEMA_INLINED_JSON}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/inline_json_schema_refs.py"
    "${CMAKE_SOURCE_DIR}/data/schemas/clipboard.schema.json"
    "${CLIPBOARD_SCHEMA_INLINED_JSON}"
  DEPENDS
    "${CMAKE_SOURCE_DIR}/data/schemas/clipboard.schema.json"
    "${CMAKE_SOURCE_DIR}/data/schemas/common.schema.json"
    "${CMAKE_SOURCE_DIR}/scripts/inline_json_schema_refs.py"
  COMMENT "Inlining shared schema definitions into clipboard.schema.json"
)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/clipboard_json_schema.h"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/generate_embedded_header.py"
    "${CLIPBOARD_SCHEMA_INLINED_JSON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/clipboard_json_schema.in.h"
    "${CMAKE_CURRENT_BINARY_DIR}/clipboard_json_schema.h"
    "@CLIPBOARD_SCHEMA_JSON_HEX_BYTES@"
  DEPENDS
    "${CLIPBOARD_SCHEMA_INLINED_JSON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/clipboard_json_schema.in.h"
    "${CMAKE_SOURCE_DIR}/scripts/generate_embedded_header.py"
  COMMENT "Generating embedded clipboard JSON schema header"
)

qt_add_library(zrythm_structure_project_lib STATIC)

target_link_libraries(zrythm_structure_project_lib
  PUBLIC
    zrythm_arrangement_lib
    zrythm_structure_tracks_lib
    zrythm_structure_scenes_lib
    zrythm_plugins_lib
    zrythm_dsp_lib
    nlohmann_json_schema_validator
    zrythm::all_compile_options
)

target_sources(zrythm_structure_project_lib
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/project_json_schema.h"
    "${CMAKE_CURRENT_BINARY_DIR}/clipboard_json_schema.h"
    arranger_tool.cpp
    audio_clip_editor.cpp
    automation_editor.cpp
    chord_editor.cpp
    chord_pad_bank.cpp
    clip_editor.cpp
    clipboard_payload.cpp
    editor_settings.cpp
    midi_editor.cpp
    port_connection.cpp
    port_connections_manager.cpp
    project.cpp
    project_graph_builder.cpp
    project_path_provider.cpp
    project_registry.cpp
    project_ui_state.cpp
    timeline_editor.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "../.."
    FILES
      arranger_tool.h
      audio_clip_editor.h
      automation_editor.h
      chord_editor.h
      chord_pad_bank.h
      clip_editor.h
      clipboard_payload.h
      editor_settings.h
      midi_editor.h
      port_connection.h
      port_connections_manager.h
      project.h
      project_graph_builder.h
      project_path_provider.h
      project_registry.h
      project_ui_state.h
      timeline_editor.h
)

set_target_properties(zrythm_structure_project_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_structure_project_lib REUSE_FROM zrythm_structure_tracks_lib)

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
set_target_properties(zrythm_structure_project_libplugin PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
