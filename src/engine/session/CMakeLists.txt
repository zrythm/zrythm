# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    control_room.h
    control_room.cpp
    exporter.h
    exporter.cpp
    graph_dispatcher.h
    graph_dispatcher.cpp
    graph_export.h
    graph_export.cpp
    midi_mapping.h
    midi_mapping.cpp
    project_graph_builder.h
    project_graph_builder.cpp
    recording_event.h
    recording_event.cpp
    recording_manager.h
    recording_manager.cpp
    sample_processor.h
    sample_processor.cpp
)
