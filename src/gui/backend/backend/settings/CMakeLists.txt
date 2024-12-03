# SPDX-FileCopyrightText: © 2019, 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    chord_preset.cpp
    chord_preset_pack.cpp
    chord_preset_pack_manager.cpp
    plugin_settings.cpp
    settings.cpp
    # g_settings_manager.cpp
    user_shortcuts.cpp
)
