# SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    action_controller.h
    action_controller.cpp
    chord_preset_manager.h
    chord_preset_manager.cpp
    alert_manager.h
    alert_manager.cpp
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
    plugin_manager.h
    plugin_manager.cpp
    plugin_protocol_paths.h
    plugin_protocol_paths.cpp
    plugin_header_qml.h
    plugin_header_qml.cpp
    project_exporter.h
    project_exporter.cpp
    project_info.h
    project_info.cpp
    project_manager.h
    project_manager.cpp
    project_session.h
    project_session.cpp
    project_templates_model.h
    project_templates_model.cpp
    qt_plugin_host_window.h
    qt_plugin_host_window.cpp
    # realtime_property.h
    recent_projects_model.h
    recent_projects_model.cpp
    resource_manager.h
    resource_manager.cpp
    translation_manager.h
    translation_manager.cpp
    ui.h
    ui.cpp
    unified_proxy_model.h
    unified_proxy_model.cpp
    x11_plugin_host_window.h
    x11_plugin_host_window.cpp
    zrythm_application.h
    zrythm_application.cpp
)

# X11 headers define macros like None/Bool/Status that break other
# translation units when unity builds batch this file together with them.
# zrythm_gui_lib is created in src/, a different directory scope than this
# file, so the property must be set in the target's scope via TARGET_DIRECTORY
# (per CMake docs: source file properties are only visible to targets added in
# the same directory).
set_source_files_properties(
  x11_plugin_host_window.cpp
  TARGET_DIRECTORY zrythm_gui_lib
  PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

add_subdirectory(io)
# add_subdirectory(gtk_widgets) # TODO delete eventually
