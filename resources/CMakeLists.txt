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

if(0)
set(out_file "${CMAKE_CURRENT_BINARY_DIR}/resources.qrc")
add_custom_command(
  OUTPUT ${out_file}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen-qt-resources-xml.py
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APPDATA_FILE}
    ${out_file}
  DEPENDS ${ui_files} appdata-xml-merged
  COMMENT "Generating resources.qrc"
)
endif()

# GL shaders
# file(GLOB gl_shaders "${CMAKE_CURRENT_SOURCE_DIR}/gl/shaders/*.glsl")

# Icons
set(icon_dirs
  arena gnome-builder gnome-icon-library fork-awesome font-awesome
  fluentui jam-icons box-icons iconpark iconoir material-design
  untitled-ui css.gg codicons
)

set(icon_files)
foreach(dir ${icon_dirs})
  file(GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/icons/${dir}/*.svg" "${CMAKE_CURRENT_SOURCE_DIR}/icons/${dir}/*.png")
  foreach(icon ${icons})
    get_filename_component(icon_name ${icon} NAME)
    set(icon_alias "icons/${dir}/${icon_name}")
    set_source_files_properties(${icon} PROPERTIES QT_RESOURCE_ALIAS ${icon_alias})
    list(APPEND icon_files ${icon})
  endforeach()
endforeach()

# add zrythm icon to resources
set(zrythm_logo "${CMAKE_SOURCE_DIR}/data/icon-themes/zrythm-dark/scalable/apps/zrythm.svg")
set_source_files_properties(${zrythm_logo} PROPERTIES QT_RESOURCE_ALIAS "icons/zrythm.svg")
list(APPEND icon_files ${zrythm_logo})

# font files
set(font_filepaths
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/Inter-4.0/InterVariable.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/Inter-4.0/InterVariable-Italic.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Regular.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Bold.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Light.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Italic.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-BoldItalic.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-LightItalic.ttf
)
set(font_files)
foreach(font_filepath ${font_filepaths})
  get_filename_component(font_name ${font_filepath} NAME)
  set_source_files_properties(${font_filepath} PROPERTIES QT_RESOURCE_ALIAS "fonts/${font_name}")
  list(APPEND font_files ${font_filepath})
endforeach()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.js.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.js"
)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/config.js" PROPERTIES QT_RESOURCE_ALIAS "config.js")

qt_add_resources(zrythm "resources"
  PREFIX "/"
  FILES
    ${ui_files}
    ${icon_files}
    ${font_files}
    ${CMAKE_CURRENT_BINARY_DIR}/config.js
)