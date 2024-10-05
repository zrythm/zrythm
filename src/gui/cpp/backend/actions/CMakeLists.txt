# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    actions.cpp
    arranger_selections.cpp
    channel_send_action.cpp
    chord_action.cpp
    midi_mapping_action.cpp
    mixer_selections_action.cpp
    port_action.cpp
    port_connection_action.cpp
    range_action.cpp
    tracklist_selections.cpp
    transport_action.cpp
    undoable_action.cpp
    undo_stack.cpp
    undo_manager.cpp)
