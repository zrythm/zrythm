# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_structure_tracks_lib STATIC)

target_link_libraries(zrythm_structure_tracks_lib
  PUBLIC
    zrythm_arrangement_lib
    zrythm_plugins_lib
    zrythm::all_compile_options
)
target_sources(zrythm_structure_tracks_lib
  PRIVATE
    audio_bus_track.h
    audio_bus_track.cpp
    audio_group_track.h
    audio_group_track.cpp
    audio_track.h
    audio_track.cpp
    automation_track.h
    automation_track.cpp
    automation_tracklist.h
    automation_tracklist.cpp
    channel.h
    channel.cpp
    channel_send.h
    channel_send.cpp
    channel_subgraph_builder.h
    channel_subgraph_builder.cpp
    chord_track.h
    chord_track.cpp
    fader.h
    fader.cpp
    foldable_track.h
    foldable_track.cpp
    folder_track.h
    folder_track.cpp
    instrument_track.h
    instrument_track.cpp
    marker_track.h
    marker_track.cpp
    master_track.h
    master_track.cpp
    midi_bus_track.h
    midi_bus_track.cpp
    midi_group_track.h
    midi_group_track.cpp
    midi_track.h
    midi_track.cpp
    modulator_track.h
    modulator_track.cpp
    piano_roll_track.h
    piano_roll_track.cpp
    recordable_track.h
    recordable_track.cpp
    track.h
    track.cpp
    track_all.h
    track_lane.h
    track_lane.cpp
    track_lane_list.h
    track_lane_list.cpp
    track_fwd.h
    track_processor.h
    track_processor.cpp
    track_routing.h
    track_routing.cpp
    track_span.h
    tracklist.h
    tracklist.cpp
)
set_target_properties(zrythm_structure_tracks_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_structure_tracks_lib
  PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/track_span.h>
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/channel_subgraph_builder.h>
)

# create a plugin so that types get exposed to QML
qt_add_qml_module(zrythm_structure_tracks_lib
  URI ZrythmTracks
  VERSION 1.0
  DEPENDENCIES
    QtCore # Needed for QAbstractItemModel-derived types
  IMPORTS
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_plugins_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmTracks
)

target_sources(zrythm_gui_lib
  PRIVATE
    track_factory.h
    track_factory.cpp
)
