# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_actions_lib STATIC)

target_sources(zrythm_actions_lib
  PRIVATE
    arranger_object_creator.cpp
    arranger_object_selection_operator.cpp
    file_importer.cpp
    qobject_property_operator.cpp
    parameter_operator.cpp
    plugin_importer.cpp
    track_creator.cpp
    track_operator.cpp
    transport_actions.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      arranger_object_creator.h
      arranger_object_selection_operator.h
      file_importer.h
      qobject_property_operator.h
      parameter_operator.h
      plugin_importer.h
      track_creator.h
      track_operator.h
      transport_actions.h
)

target_include_directories(zrythm_actions_lib
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(zrythm_actions_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
  VERIFY_INTERFACE_HEADER_SETS ${ZRYTHM_VERIFY_INTERFACE_HEADER_SETS}
)

target_precompile_headers(zrythm_actions_lib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/track_operator.h>)

target_link_libraries(zrythm_actions_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_undo_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_actions_lib
  URI ZrythmActions
  VERSION 1.0
  IMPORTS
    TARGET zrythm_utils_lib
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_structure_scenes_lib
    TARGET zrythm_structure_tracks_lib
    TARGET zrythm_undo_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmActions
)
