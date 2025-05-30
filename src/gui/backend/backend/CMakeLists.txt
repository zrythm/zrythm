# SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    audio_clip_editor.cpp
    automation_editor.cpp
    chord_editor.cpp
    clip_editor.cpp
    clipboard.cpp
    editor_settings.cpp
    file_manager.cpp
    piano_roll.cpp
    project.cpp
    # project/project_init_flow_manager.cpp
    project_info.cpp
    settings_manager.h
    settings_manager.cpp
    theme_manager.h
    theme_manager.cpp
    timeline.cpp
    zrythm.h
    zrythm.cpp
    )

add_subdirectory(actions)
add_subdirectory(settings)
