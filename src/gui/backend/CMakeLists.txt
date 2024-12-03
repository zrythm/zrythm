# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    alert_manager.h
    alert_manager.cpp
    cached_plugin_descriptors.h
    cached_plugin_descriptors.cpp
    carla_discovery.h
    carla_discovery.cpp
    channel.h
    channel.cpp
    curve_preset.h
    curve_preset.cpp
    plugin_collections.h
    plugin_collections.cpp
    plugin_descriptor.h
    plugin_descriptor_list.h
    plugin_manager.h
    plugin_manager.cpp
    plugin_protocol_paths.h
    plugin_protocol_paths.cpp
    plugin_scanner.h
    plugin_scanner.cpp
    project_manager.h
    project_manager.cpp
    ui.h
    ui.cpp
)

add_subdirectory(backend)
add_subdirectory(io)
# add_subdirectory(gtk_widgets) # TODO delete eventually
