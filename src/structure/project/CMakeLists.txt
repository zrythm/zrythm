# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
    project.cpp
    project_graph_builder.cpp
    project_path_provider.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "../.."
    FILES
      project.h
      project_graph_builder.h
      project_path_provider.h
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
