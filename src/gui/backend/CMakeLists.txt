# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    alert_manager.h
    alert_manager.cpp
    carla_discovery.h
    carla_discovery.cpp
    curve_preset.h
    curve_preset.cpp
    device_manager.h
    device_manager.cpp
    plugin_collections.h
    plugin_collections.cpp
    plugin_manager.h
    plugin_manager.cpp
    plugin_protocol_paths.h
    plugin_protocol_paths.cpp
    project_manager.h
    project_manager.cpp
    ui.h
    ui.cpp
)

add_subdirectory(backend)
add_subdirectory(io)
# add_subdirectory(gtk_widgets) # TODO delete eventually
