# SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    arranger_selections.cpp
    audio_clip_editor.cpp
    audio_selections.cpp
    automation_editor.cpp
    automation_selections.cpp
    chord_editor.cpp
    chord_selections.cpp
    clip_editor.cpp
    clipboard.cpp
    editor_settings.cpp
    file_manager.cpp
    midi_selections.cpp
    mixer_selections.cpp
    piano_roll.cpp
    project.cpp
    project/project_init_flow_manager.cpp
    project_info.cpp
    settings_manager.h
    settings_manager.cpp
    theme_manager.h
    theme_manager.cpp
    timeline.cpp
    timeline_selections.cpp
    tracklist_selections.cpp
    zrythm.h
    zrythm.cpp
    )

add_subdirectory(actions)
if(ZRYTHM_YAML_PROJECT_COMPATIBILITY)
  add_subdirectory(cyaml_schemas)
endif()
add_subdirectory(settings)
