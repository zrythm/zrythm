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
    audio_bus_track.cpp
    audio_group_track.cpp
    audio_track.cpp
    automation_track.cpp
    automation_tracklist.cpp
    channel.cpp
    channel_send.cpp
    channel_subgraph_builder.cpp
    chord_track.cpp
    clip_playback_data_provider.cpp
    folder_track.cpp
    instrument_track.cpp
    marker_track.cpp
    master_track.cpp
    midi_bus_track.cpp
    midi_group_track.cpp
    midi_track.cpp
    modulator_track.cpp
    piano_roll_track.cpp
    playback_cache_activity_aggregator.cpp
    playback_cache_activity_tracker.cpp
    recordable_track.cpp
    track.cpp
    track_collection.cpp
    track_lane.cpp
    track_lane_list.cpp
    track_processor.cpp
    track_routing.cpp
    tracklist.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ".."
    FILES
      audio_bus_track.h
      audio_group_track.h
      audio_track.h
      automation_track.h
      automation_tracklist.h
      channel.h
      channel_send.h
      channel_subgraph_builder.h
      chord_track.h
      clip_playback_data_provider.h
      folder_track.h
      instrument_track.h
      marker_track.h
      master_track.h
      midi_bus_track.h
      midi_group_track.h
      midi_track.h
      modulator_track.h
      piano_roll_track.h
      playback_cache_activity_aggregator.h
      playback_cache_activity_tracker.h
      recordable_track.h
      singleton_tracks.h
      track.h
      track_all.h
      track_collection.h
      track_factory.h
      track_fwd.h
      track_lane.h
      track_lane_list.h
      track_processor.h
      track_routing.h
      track_span.h
      tracklist.h
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
    TARGET zrythm_utils_lib
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_plugins_lib
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ZrythmTracks
)
