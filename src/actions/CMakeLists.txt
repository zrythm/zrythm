# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_actions_lib STATIC)

target_sources(zrythm_actions_lib
  PRIVATE
    parameter_operator.h
    parameter_operator.cpp
    track_operator.h
    track_operator.cpp
)

target_include_directories(zrythm_actions_lib
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(zrythm_actions_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
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
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_structure_tracks_lib
    TARGET zrythm_undo_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmActions
)
