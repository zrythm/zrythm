# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    action_controller.h
    action_controller.cpp
    alert_manager.h
    alert_manager.cpp
    arranger_tool.h
    arranger_tool.cpp
    automation_tracklist_proxy_model.h
    automation_tracklist_proxy_model.cpp
    # carla_discovery.h
    # carla_discovery.cpp
    cursor_manager.h
    cursor_manager.cpp
    curve_preset.h
    curve_preset.cpp
    device_manager.h
    device_manager.cpp
    file_system_model.h
    file_system_model.cpp
    global_state.h
    plugin_collections.h
    plugin_collections.cpp
    plugin_host_window.h
    plugin_host_window.cpp
    plugin_manager.h
    plugin_manager.cpp
    plugin_protocol_paths.h
    plugin_protocol_paths.cpp
    project_exporter.h
    project_exporter.cpp
    project_info.h
    project_info.cpp
    project_manager.h
    project_manager.cpp
    project_path_provider.h
    project_path_provider.cpp
    project_templates_model.h
    project_templates_model.cpp
    project_ui_state.h
    project_ui_state.cpp
    # realtime_property.h
    recent_projects_model.h
    recent_projects_model.cpp
    resource_manager.h
    resource_manager.cpp
    track_filter_proxy_model.h
    track_filter_proxy_model.cpp
    translation_manager.h
    translation_manager.cpp
    ui.h
    ui.cpp
    unified_proxy_model.h
    unified_proxy_model.cpp
    zrythm_application.h
    zrythm_application.cpp
)

add_subdirectory(backend)
add_subdirectory(io)
# add_subdirectory(gtk_widgets) # TODO delete eventually
