# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    audio_bus_track.h
    audio_bus_track.cpp
    audio_group_track.h
    audio_group_track.cpp
    audio_lane.h
    audio_lane.cpp
    audio_track.h
    audio_track.cpp
    automatable_track.h
    automatable_track.cpp
    automation_track.h
    automation_track.cpp
    automation_tracklist.h
    automation_tracklist.cpp
    channel.h
    channel.cpp
    channel_send.h
    channel_send.cpp
    channel_track.h
    channel_track.cpp
    chord_track.h
    chord_track.cpp
    fader.h
    fader.cpp
    foldable_track.h
    foldable_track.cpp
    folder_track.h
    folder_track.cpp
    group_target_track.h
    group_target_track.cpp
    instrument_track.h
    instrument_track.cpp
    laned_track.h
    laned_track.cpp
    marker_track.h
    marker_track.cpp
    master_track.h
    master_track.cpp
    midi_bus_track.h
    midi_bus_track.cpp
    midi_group_track.h
    midi_group_track.cpp
    midi_lane.h
    midi_lane.cpp
    midi_track.h
    midi_track.cpp
    modulator_track.h
    modulator_track.cpp
    piano_roll_track.h
    piano_roll_track.cpp
    processable_track.h
    processable_track.cpp
    recordable_track.h
    recordable_track.cpp
    tempo_track.h
    tempo_track.cpp
    track.h
    track.cpp
    track_all.h
    track_factory.h
    track_fwd.h
    track_lane.h
    track_lane.cpp
    track_lane_list.h
    track_lane_list.cpp
    track_processor.h
    track_processor.cpp
    track_span.h
    track_span.cpp
    tracklist.h
    tracklist.cpp
)

qt_add_library(zrythm_structure_tracks_lib STATIC)

target_sources(zrythm_structure_tracks_lib
  PRIVATE
    port_connection.h
    port_connection.cpp
    port_connections_manager.h
    port_connections_manager.cpp
    port_identifier.h
    port_identifier.cpp
)

set_target_properties(zrythm_structure_tracks_lib PROPERTIES
  UNITY_BUILD ${ZRYTHM_UNITY_BUILD}
)

target_precompile_headers(zrythm_structure_tracks_lib REUSE_FROM zrythm_dsp_lib)

target_link_libraries(zrythm_structure_tracks_lib
  PUBLIC
    ${zrythm_link_libs}
    zrythm_dsp_lib
    zrythm::include_dirs
    zrythm::all_compile_options
)