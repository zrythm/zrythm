# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(blp_filenames
  ui/chord_selector_window.blp
  ui/project_scale_popover.blp
  ui/audio_editor_space.blp
  ui/automation_editor_space.blp
  ui/active_hardware_popover.blp
  ui/bug_report_dialog.blp
  ui/main_window.blp
  ui/cc-list-row-info-button.blp
  ui/chord_editor_space.blp
  ui/fader_buttons.blp
  ui/snap_box.blp
  ui/snap_grid_popover.blp
  ui/timeline_panel.blp
  ui/folder_channel.blp
  ui/transport_controls.blp
  ui/event_viewer.blp
  ui/expander_box.blp
  ui/editor_toolbar.blp
  ui/project_toolbar.blp
  ui/fishbowl_window.blp
  ui/scale_selector_window.blp
  ui/bot_bar.blp
  ui/inspector_track.blp
  ui/chord_pad_panel.blp
  ui/center_dock.blp
  ui/automatable_selector.blp
  ui/string_entry_dialog.blp
  ui/range_action_buttons.blp
  ui/right_dock_edge.blp
  ui/timeline_toolbar.blp
  ui/monitor_section.blp
  ui/modulator_view.blp
  ui/log_viewer.blp
  ui/quantize_box.blp
  ui/track.blp
  ui/chord_key.blp
  ui/control_room.blp
  ui/fader_controls_grid.blp
  ui/playhead_scroll_buttons.blp
  ui/clip_editor.blp
  ui/left_dock_edge.blp
  ui/zoom_buttons.blp
  ui/bind_cc_dialog.blp
  ui/clip_editor_inner.blp
  ui/modulator_macro.blp
  ui/tracklist_header.blp
  ui/toolbox.blp
  ui/greeter.blp
  ui/file_auditioner_controls.blp
  ui/mixer.blp
  ui/channel.blp
  ui/panel_file_browser.blp
  ui/plugin_browser.blp
  ui/quantize_dialog.blp
  ui/inspector_plugin.blp
  ui/modulator_inner.blp
  ui/export_dialog.blp
  ui/port_selector_dialog.blp
  ui/midi_editor_space.blp
  ui/bounce_dialog.blp
  ui/chord_pack_browser.blp
  ui/generic_progress_dialog.blp
  ui/file_browser_filters.blp
  gtk/menus.blp
  gtk/help-overlay.blp
)

set(ui_files)
foreach(blp ${blp_filenames})
  string(REPLACE ".blp" ".ui" ui_file ${blp})
  list(APPEND ui_files ${ui_file})
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zrythm.gresources.xml
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen-gtk-resources-xml.py
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APPDATA_FILE}
    ${CMAKE_CURRENT_BINARY_DIR}/zrythm.gresources.xml
  DEPENDS ${ui_files} appdata-xml-merged
  COMMENT "Generating zrythm.gresources.xml"
)

find_program(GLIB_COMPILE_RESOURCES glib-compile-resources
  PATHS ${GLIB_COMPILE_RESOURCES_PATH}
  REQUIRED)

set(GRESOURCE_COMPILED_FILE "${CMAKE_CURRENT_BINARY_DIR}/zrythm-resources.c")

add_custom_command(
  OUTPUT "${GRESOURCE_COMPILED_FILE}"
  COMMAND ${GLIB_COMPILE_RESOURCES}
    "--target=${GRESOURCE_COMPILED_FILE}"
    --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}
    --sourcedir=${CMAKE_CURRENT_BINARY_DIR}
    --sourcedir=${CMAKE_BINARY_DIR}/data
    --generate-source
    --c-name=zrythm
    "${CMAKE_CURRENT_BINARY_DIR}/zrythm.gresources.xml"
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zrythm.gresources.xml ${ui_files} appdata-xml-merged
  COMMENT "Compiling GResource file: zrythm-resources.c"
)

add_library(zrythm-gresources STATIC ${GRESOURCE_COMPILED_FILE})
if(compiler_gnulike)
  target_compile_options(zrythm-gresources
    PRIVATE -Wno-overlength-strings)  
endif()
target_link_libraries(zrythm-gresources PRIVATE PkgConfig::GIOMM)