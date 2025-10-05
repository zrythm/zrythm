# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_structure_scenes_lib STATIC)

target_link_libraries(zrythm_structure_scenes_lib
  PUBLIC
    zrythm_structure_tracks_lib
    zrythm::all_compile_options
)
target_sources(zrythm_structure_scenes_lib
  PRIVATE
    clip_launcher.h
    clip_launcher.cpp
    clip_playback_service.h
    clip_playback_service.cpp
    clip_slot.h
    clip_slot.cpp
    scene.h
    scene.cpp
)
set_target_properties(zrythm_structure_scenes_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_structure_scenes_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/clip_launcher.h>
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_structure_scenes_lib
  URI ZrythmScenes
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_structure_tracks_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmScenes
)
