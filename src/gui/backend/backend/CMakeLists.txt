# SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    clip_editor.cpp
    clipboard.cpp
    file_manager.cpp
    project.cpp
    # project/project_init_flow_manager.cpp
    settings_manager.h
    settings_manager.cpp
    theme_manager.h
    theme_manager.cpp
    zrythm.h
    zrythm.cpp
    )

# add_subdirectory(actions)
add_subdirectory(settings)
