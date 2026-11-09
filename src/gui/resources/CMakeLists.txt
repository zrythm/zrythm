# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(blp_filenames
  ui/bug_report_dialog.blp
  ui/cc-list-row-info-button.blp
  ui/fader_buttons.blp
  ui/folder_channel.blp
  ui/transport_controls.blp
  ui/event_viewer.blp
  ui/editor_toolbar.blp
  ui/bot_bar.blp
  ui/inspector_track.blp
  ui/automatable_selector.blp
  ui/string_entry_dialog.blp
  ui/range_action_buttons.blp
  ui/timeline_toolbar.blp
  ui/log_viewer.blp
  ui/quantize_box.blp
  ui/track.blp
  ui/chord_key.blp
  ui/playhead_scroll_buttons.blp
  ui/zoom_buttons.blp
  ui/bind_cc_dialog.blp
  ui/clip_editor_inner.blp
  ui/tracklist_header.blp
  ui/greeter.blp
  ui/file_auditioner_controls.blp
  ui/mixer.blp
  ui/channel.blp
  ui/panel_file_browser.blp
  ui/plugin_browser.blp
  ui/quantize_dialog.blp
  ui/inspector_plugin.blp
  ui/export_dialog.blp
  ui/port_selector_dialog.blp
  ui/bounce_dialog.blp
  ui/chord_pack_browser.blp
  ui/file_browser_filters.blp
)

set(ui_files)
foreach(blp ${blp_filenames})
  string(REPLACE ".blp" ".ui" ui_file ${blp})
  list(APPEND ui_files ${ui_file})
endforeach()

# GL shaders
# file(GLOB gl_shaders "${CMAKE_CURRENT_SOURCE_DIR}/gl/shaders/*.glsl")
