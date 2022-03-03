/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#include "project.h"
#include "actions/actions.h"
#include "audio/master_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/accel.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_toolbar.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/header.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/text_expander.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/tracklist.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/system.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  MainWindowWidget, main_window_widget,
  GTK_TYPE_APPLICATION_WINDOW)

/**
 * This is called when the window closing is
 * finalized and cannot be intercepted.
 */
static void
on_main_window_destroy (
  MainWindowWidget * self,
  gpointer user_data)
{
  g_message ("main window destroy %p", self);

  event_manager_process_now (EVENT_MANAGER);

  /* if this is the current main window and a project
   * is loaded, quit the application (check needed
   * because this is also called right after loading
   * a project) */
  if (PROJECT->loaded
      && zrythm_app->main_window == self)
    {
      event_manager_stop_events (EVENT_MANAGER);

      object_free_w_func_and_null (
        project_free, PROJECT);

#if 0
      /* stop engine */
      engine_activate (AUDIO_ENGINE, false);

      /* stop events from getting fired. this
       * prevents some segfaults on shutdown */
      event_manager_stop_events (EVENT_MANAGER);

      /* close any plugin windows */
#endif

      g_application_quit (
        G_APPLICATION (zrythm_app));
    }

  g_message ("main window destroy called");
}

/**
 * This is called when a close request is handled
 * and can be intercepted.
 */
static bool
on_close_request (
  GtkWindow *        window,
  MainWindowWidget * self)
{
  g_debug (
    "%s: main window delete event called", __func__);

  /* temporarily pause engine */
  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, Z_F_NO_FORCE);

  /* get yaml for live project */
  char * live_yaml =
    yaml_serialize (PROJECT, &project_schema);
  if (!live_yaml)
    {
      ui_show_error_message (
        self, false,
        _("Failed to serialize current project"));

      /* restart engine */
      engine_resume (AUDIO_ENGINE, &state);

      /* return true to stop other handlers from
       * running */
      return true;
    }

  /* get yaml for existing project file */
  char * file_yaml =
    project_get_existing_yaml (
      PROJECT, F_NOT_BACKUP);

  /* ask for save if project has unsaved changes */
  bool ret = false;
  if (!string_is_equal (file_yaml, live_yaml))
    {
      bool show_diff = false;
      char * diff = NULL;

      /* no need, usually diff is too large so this
       * is a waste of time */
#if 0
#ifndef _WOE32
      /* get diff using `diff` */
      const size_t max_diff_len = 400;
      GError * err = NULL;
      char * tmp_dir =
        g_dir_make_tmp ("zrythm-diff-XXXXXX", &err);
      g_return_val_if_fail (err == NULL, false);
      char * tmp_live_fp =
        g_build_filename (
          tmp_dir, "live.yaml", NULL);
      char * tmp_file_fp =
        g_build_filename (
          tmp_dir, "file.yaml", NULL);
      err = NULL;
      g_file_set_contents (
        tmp_live_fp, live_yaml, -1, &err);
      g_return_val_if_fail (err == NULL, false);
      err = NULL;
      g_file_set_contents (
        tmp_file_fp, file_yaml, -1, &err);
      g_return_val_if_fail (err == NULL, false);

      char * diff_bin =
        g_find_program_in_path ("diff");
      diff = NULL;
      if (diff_bin)
        {
          const char * argv[] = {
            diff_bin, tmp_live_fp, tmp_file_fp, NULL };
          diff =
            system_get_cmd_output (
              (char **) argv, -1, false);
          g_debug ("diff: %s", diff);
          g_free (diff_bin);
        }
      show_diff =
        diff && strlen (diff) < max_diff_len;

      io_remove (tmp_live_fp);
      io_remove (tmp_file_fp);
      io_rmdir (tmp_dir, F_NO_FORCE);
      g_free (tmp_dir);
      g_free (tmp_live_fp);
      g_free (tmp_file_fp);
#endif
#endif

      GtkWidget * dialog =
        gtk_dialog_new_with_buttons (
          _("Unsaved changes"),
          GTK_WINDOW (MAIN_WINDOW),
          GTK_DIALOG_MODAL |
            GTK_DIALOG_DESTROY_WITH_PARENT,
          _("_Save & Quit"),
          GTK_RESPONSE_ACCEPT,
          _("_Quit without saving"),
          GTK_RESPONSE_REJECT,
          _("_Cancel"),
          GTK_RESPONSE_CANCEL,
          NULL);
      gtk_dialog_set_default_response (
        GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
      const char * label_question =
        _("The project contains unsaved changes.\n"
        "If you quit without saving, unsaved "
        "changes will be lost.");
      char label_str[4000];
      sprintf (
        label_str,
        show_diff ? _("%s\n\nChanges:\n%s") : "%s%s",
        label_question,
        show_diff ? diff : "");
      GtkWidget * label = gtk_label_new (label_str);
      gtk_widget_set_margin_top (label, 4);
      gtk_widget_set_margin_bottom (label, 4);
      gtk_widget_set_visible (label, true);
      GtkWidget * content =
        gtk_dialog_get_content_area (
          GTK_DIALOG (dialog));
      gtk_box_append (
        GTK_BOX (content), label);
      int dialog_res =
        z_gtk_dialog_run (
          GTK_DIALOG (dialog), true);
      switch (dialog_res)
        {
        case GTK_RESPONSE_ACCEPT:
          /* save project */
#ifdef TRIAL_VER
          g_message ("skipping save project...");
#else
          g_message ("saving project...");
          project_save (
            PROJECT, PROJECT->dir, F_NOT_BACKUP,
            ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC);
#endif
          break;
        case GTK_RESPONSE_REJECT:
          /* no action needed - just quit */
          g_message ("quitting without saving...");
          break;
        default:
          /* return true to cancel */
          g_message ("cancel");
          ret = true;
          break;
        }

      if (diff)
        g_free (diff);

    } /* endif project has unsaved changes */
  g_free (live_yaml);
  free (file_yaml);

  /* restart engine */
  engine_resume (AUDIO_ENGINE, &state);

  g_debug (
    "%s: main window delete event returning %d",
    __func__, ret);

  return ret;
}

MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  MainWindowWidget * self = g_object_new (
    MAIN_WINDOW_WIDGET_TYPE,
    "application", G_APPLICATION (_app),
    "title", PROGRAM_NAME,
    NULL);

  return self;
}

static gboolean
on_key_pressed (
  GtkEventControllerKey* self,
  guint keyval,
  guint keycode,
  GdkModifierType state,
  gpointer user_data)
{
  g_debug ("main window key press");

  if (!z_gtk_keyval_is_arrow (keyval))
    return false;

  if (MW_TRACK_INSPECTOR->comment->has_focus)
    {
      return false;
    }

#if 0
  if (Z_IS_ARRANGER_WIDGET (
        MAIN_WINDOW->last_focused))
    {
      arranger_widget_on_key_action (
        widget, event,
        Z_ARRANGER_WIDGET (
          MAIN_WINDOW->last_focused));

      /* stop other handlers */
      return TRUE;
    }
#endif

  return false;
}

static bool
show_startup_errors (
  MainWindowWidget * self)
{
  /* show any startup errors */
  for (int k = 0;
       k < zrythm_app->num_startup_errors;
       k++)
    {
      char * msg =
        zrythm_app->startup_errors[k];
      ui_show_error_message (
        self, true, msg);
      g_free (msg);
    }
  zrythm_app->num_startup_errors = 0;

  return G_SOURCE_REMOVE;
}

void
main_window_widget_setup (
  MainWindowWidget * self)
{
  g_return_if_fail (self);

  g_message ("Setting up...");

  if (self->setup)
    {
      g_message ("already set up");
      return;
    }

  header_widget_setup (self->header, PROJECT->title);

  /* setup center dock */
  center_dock_widget_setup (self->center_dock);

  editor_toolbar_widget_setup (
    MW_EDITOR_TOOLBAR);

  /* setup piano roll */
  if (MW_BOT_DOCK_EDGE && MW_CLIP_EDITOR)
    {
      clip_editor_widget_setup (
        MW_CLIP_EDITOR);
    }

  // set icons
  /*gtk_window_set_icon_name (*/
    /*GTK_WINDOW (self), "zrythm");*/

  /* setup top and bot bars */
  top_bar_widget_refresh (self->top_bar);
  bot_bar_widget_setup (self->bot_bar);

  /* setup mixer */
  g_warn_if_fail (
    P_MASTER_TRACK && P_MASTER_TRACK->channel);
  mixer_widget_setup (
    MW_MIXER, P_MASTER_TRACK->channel);

  gtk_window_maximize (GTK_WINDOW (self));

  /* show track selection info */
  g_warn_if_fail (TRACKLIST_SELECTIONS->tracks[0]);
  EVENTS_PUSH (ET_TRACK_CHANGED,
               TRACKLIST_SELECTIONS->tracks[0]);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, TL_SELECTIONS);
  event_viewer_widget_refresh (
    MW_TIMELINE_EVENT_VIEWER, false);

  EVENTS_PUSH (
    ET_MAIN_WINDOW_LOADED, NULL);

  g_idle_add (
    (GSourceFunc) show_startup_errors, self);

  gtk_window_present (GTK_WINDOW (self));

  self->setup = true;

  event_manager_process_now (EVENT_MANAGER);

  g_message ("done");
}

/**
 * Prepare for finalization.
 */
void
main_window_widget_tear_down (
  MainWindowWidget * self)
{
  g_message ("tearing down main window %p...", self);

  self->setup = false;

  if (self->center_dock)
    {
      center_dock_widget_tear_down (
        self->center_dock);
    }

  gtk_window_set_application (
    GTK_WINDOW (self), NULL);

  g_message ("done tearing down main window");
}

static void
main_window_finalize (
  MainWindowWidget * self)
{
  g_message ("finalizing main_window...");

  G_OBJECT_CLASS (
    main_window_widget_parent_class)->
      finalize (G_OBJECT (self));

  g_message ("done");
}

static void
main_window_widget_class_init (
  MainWindowWidgetClass * klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (klass);
  resources_set_class_template (
    wklass, "main_window.ui");
  gtk_widget_class_set_css_name (
    wklass, "main-window");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    wklass, MainWindowWidget, x)

  BIND_CHILD (toast_overlay);
  BIND_CHILD (main_box);
  BIND_CHILD (header);
  BIND_CHILD (top_bar);
  BIND_CHILD (center_box);
  BIND_CHILD (center_dock);
  BIND_CHILD (bot_bar);
  gtk_widget_class_bind_template_callback (
    klass,
    on_main_window_destroy);

#undef BIND_CHILD

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) main_window_finalize;
}

static void
main_window_widget_init (MainWindowWidget * self)
{
  g_message ("Initing main window widget...");

  GActionEntry actions[] = {

    /* file menu */
    { "new", activate_new },
    { "open", activate_open },
    { "save", activate_save },
    { "save-as", activate_save_as },
    { "export-as", activate_export_as },
    { "export-graph", activate_export_graph },
    { "properties", activate_properties },

    /* edit menu */
    { "undo", activate_undo },
    { "redo", activate_redo },
    { "undo_n", activate_undo_n, "i" },
    { "redo_n", activate_redo_n, "i" },
    { "cut", activate_cut },
    { "copy", activate_copy },
    { "paste", activate_paste },
    { "delete", activate_delete },
    { "duplicate", activate_duplicate },
    { "clear-selection", activate_clear_selection },
    { "select-all", activate_select_all },
    /* selection submenu */
    { "loop-selection", activate_loop_selection },
    { "mute-selection", activate_mute_selection,
      "s"},

    /* view menu */
    { "toggle-left-panel",
      activate_toggle_left_panel },
    { "toggle-right-panel",
      activate_toggle_right_panel },
    { "toggle-bot-panel",
      activate_toggle_bot_panel },
    { "toggle-top-panel",
      activate_toggle_top_panel },
    { "toggle-status-bar",
      activate_toggle_status_bar },
    { "zoom-in", activate_zoom_in },
    { "zoom-out", activate_zoom_out },
    { "original-size", activate_original_size },
    { "best-fit", activate_best_fit },

    /* snapping, quantize */
    { "snap-to-grid", activate_snap_to_grid, "s" },
    { "snap-keep-offset",
      activate_snap_keep_offset, "s" },
    { "snap-events", activate_snap_events, "s" },
    { "quick-quantize",
      activate_quick_quantize, "s"},
    { "quantize-options", activate_quantize_options,
      "s" },

    /* range actions */
    { "insert-silence", activate_insert_silence },
    { "remove-range", activate_remove_range },

    /* playhead actions */
    { "timeline-playhead-scroll-edges", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "timeline-playhead-scroll-edges") ?
        "true" : "false",
      change_state_timeline_playhead_scroll_edges },
    { "timeline-playhead-follow", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "timeline-playhead-follow") ?
        "true" : "false",
      change_state_timeline_playhead_follow },
    { "editor-playhead-scroll-edges", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "editor-playhead-scroll-edges") ?
        "true" : "false",
      change_state_editor_playhead_scroll_edges },
    { "editor-playhead-follow", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "editor-playhead-follow") ?
        "true" : "false",
      change_state_editor_playhead_follow },

    /* merge actions */
    { "merge-selection", activate_merge_selection },

    /* musical mode */
    { "toggle-musical-mode", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "musical-mode") ?
        "true" : "false",
      change_state_musical_mode },

    /* track actions */
    { "create-audio-track",
      activate_create_audio_track },
    { "create-audio-bus-track",
      activate_create_audio_bus_track },
    { "create-midi-bus-track",
      activate_create_midi_bus_track },
    { "create-midi-track",
      activate_create_midi_track },
    { "create-audio-group-track",
      activate_create_audio_group_track },
    { "create-midi-group-track",
      activate_create_midi_group_track },
    { "create-folder-track",
      activate_create_folder_track },
    { "add-region", activate_add_region },

    /* modes */
    { "select-mode", activate_select_mode },
    { "edit-mode", activate_edit_mode },
    { "cut-mode", activate_cut_mode },
    { "eraser-mode", activate_eraser_mode },
    { "ramp-mode", activate_ramp_mode },
    { "audition-mode", activate_audition_mode },

    /* transport */
    { "toggle-metronome", NULL, NULL,
      g_settings_get_boolean (
        S_TRANSPORT, "metronome-enabled") ?
        "true" : "false",
      change_state_metronome },
    { "toggle-loop", NULL, NULL,
      g_settings_get_boolean (
        S_TRANSPORT, "loop") ? "true" : "false",
      change_state_loop },
    { "goto-prev-marker",
      activate_goto_prev_marker },
    { "goto-next-marker",
      activate_goto_next_marker },
    { "play-pause", activate_play_pause },
    { "go-to-start", activate_go_to_start },
    { "input-bpm", activate_input_bpm },
    { "tap-bpm", activate_tap_bpm },

    /* transport - jack */
    { "set-timebase-master",
      activate_set_timebase_master },
    { "set-transport-client",
      activate_set_transport_client },
    { "unlink-jack-transport",
      activate_unlink_jack_transport },

    /* tracks */
    { "delete-selected-tracks",
      activate_delete_selected_tracks },
    { "duplicate-selected-tracks",
      activate_duplicate_selected_tracks },
    { "hide-selected-tracks",
      activate_hide_selected_tracks },
    { "pin-selected-tracks",
      activate_pin_selected_tracks },
    { "solo-selected-tracks",
      activate_solo_selected_tracks },
    { "unsolo-selected-tracks",
      activate_unsolo_selected_tracks },
    { "mute-selected-tracks",
      activate_mute_selected_tracks },
    { "unmute-selected-tracks",
      activate_unmute_selected_tracks },
    { "listen-selected-tracks",
      activate_listen_selected_tracks },
    { "unlisten-selected-tracks",
      activate_unlisten_selected_tracks },
    { "enable-selected-tracks",
      activate_enable_selected_tracks },
    { "disable-selected-tracks",
      activate_disable_selected_tracks },
    { "change-track-color",
      activate_change_track_color },
    { "track-set-midi-channel",
      activate_track_set_midi_channel, "s" },
    { "quick-bounce-selected-tracks",
      activate_quick_bounce_selected_tracks },
    { "bounce-selected-tracks",
      activate_bounce_selected_tracks },
    { "selected-tracks-direct-out-to",
      activate_selected_tracks_direct_out_to, "i" },
    { "selected-tracks-direct-out-new",
      activate_selected_tracks_direct_out_new, },
    { "toggle-track-passthrough-input",
      activate_toggle_track_passthrough_input, },

    /* piano roll */
    { "toggle-drum-mode", NULL, NULL, "false",
      change_state_piano_roll_drum_mode },
    { "toggle-listen-notes", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "listen-notes") ?
        "true" : "false",
      change_state_listen_notes },
    { "midi-editor.highlighting",
      activate_midi_editor_highlighting, "s" },

    /* automation */
    { "show-automation-values", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "show-automation-values") ?
        "true" : "false",
      change_state_show_automation_values },

    /* control room */
    { "toggle-dim-output", NULL, NULL,
      "true", change_state_dim_output },

    /* file browser */
    { "show-file-browser",
      activate_show_file_browser },

    /* show/hide event viewers */
    { "toggle-timeline-event-viewer",
      activate_toggle_timeline_event_viewer },
    { "toggle-editor-event-viewer",
      activate_toggle_editor_event_viewer },

    /* editor functions */
    { "editor-function", activate_editor_function,
      "s" },
    { "editor-function-lv2",
      activate_editor_function_lv2, "s" },

    /* rename track/region */
    { "rename-track-or-region",
      activate_rename_track_or_region },

    /* arranger selections */
    { "nudge-selection",
      activate_nudge_selection, "s" },
    { "detect-bpm",
      activate_detect_bpm, "s" },
    { "timeline-function",
      activate_timeline_function, "i" },
    { "quick-bounce-selections",
      activate_quick_bounce_selections },
    { "bounce-selections",
      activate_bounce_selections },
    { "set-curve-algorithm",
      activate_set_curve_algorithm, "i", },
    { "set-region-fade-in-algorithm-preset",
      activate_set_region_fade_in_algorithm_preset,
      "s", },
    { "set-region-fade-out-algorithm-preset",
      activate_set_region_fade_out_algorithm_preset,
      "s", },
    { "arranger-object-view-info",
      activate_arranger_object_view_info, "s" },

    /* chord presets */
    { "save-chord-preset",
      activate_save_chord_preset, },
    { "load-chord-preset",
      activate_load_chord_preset, "s" },
    { "load-chord-preset-from-scale",
      activate_load_chord_preset_from_scale, "s" },
    { "transpose-chord-pad",
      activate_transpose_chord_pad, "s" },
    { "add-chord-preset-pack",
      activate_add_chord_preset_pack, },
    { "delete-chord-preset-pack",
      activate_delete_chord_preset_pack, "s" },
    { "rename-chord-preset-pack",
      activate_rename_chord_preset_pack, "s" },
    { "delete-chord-preset",
      activate_delete_chord_preset, "s" },
    { "rename-chord-preset",
      activate_rename_chord_preset, "s" },

    /* cc bindings */
    { "bind-midi-cc",
      activate_bind_midi_cc, "s" },
    { "delete-cc-binding",
      activate_delete_cc_binding, "i" },

    /* port actions */
    { "reset-stereo-balance",
      activate_reset_stereo_balance, "s" },
    { "reset-fader",
      activate_reset_fader, "s" },
    { "reset-control",
      activate_reset_control, "s" },
    { "port-view-info",
      activate_port_view_info, "s" },
    { "port-connection-remove",
      activate_port_connection_remove },

    /* plugin actions */
    { "plugin-toggle-enabled",
      activate_plugin_toggle_enabled, "s" },
    { "plugin-inspect",
      activate_plugin_inspect },

    /* panel file browser actions */
    { "panel-file-browser-add-bookmark",
      activate_panel_file_browser_add_bookmark,
      "s" },
    { "panel-file-browser-delete-bookmark",
      activate_panel_file_browser_delete_bookmark },

    /* pluginbrowser actions */
    { "plugin-browser-add-to-project",
      activate_plugin_browser_add_to_project, "s" },
    { "plugin-browser-add-to-project-carla",
      activate_plugin_browser_add_to_project_carla,
      "s"},
    { "plugin-browser-add-to-project-bridged-ui",
      activate_plugin_browser_add_to_project_bridged_ui,
      "s" },
    { "plugin-browser-add-to-project-bridged-full",
      activate_plugin_browser_add_to_project_bridged_full,
      "s" },
    { "plugin-browser-toggle-generic-ui",
      NULL, NULL, "false",
      change_state_plugin_browser_toggle_generic_ui },
    { "plugin-browser-add-to-collection",
      activate_plugin_browser_add_to_collection,
      "s"},
    { "plugin-browser-remove-from-collection",
      activate_plugin_browser_remove_from_collection,
      "s"},
    { "plugin-browser-reset",
      activate_plugin_browser_reset, "s" },
    { "plugin-collection-add",
      activate_plugin_collection_add, },
    { "plugin-collection-rename",
      activate_plugin_collection_rename, },
    { "plugin-collection-remove",
      activate_plugin_collection_remove, },
  };

#if 0
  g_action_map_add_action_entries (
    G_ACTION_MAP (self), actions,
    G_N_ELEMENTS (actions), self);
#endif
  g_action_map_add_action_entries (
    G_ACTION_MAP (zrythm_app), actions,
    G_N_ELEMENTS (actions), zrythm_app);

  g_type_ensure (HEADER_WIDGET_TYPE);
  g_type_ensure (TOP_BAR_WIDGET_TYPE);
  g_type_ensure (CENTER_DOCK_WIDGET_TYPE);
  g_type_ensure (BOT_BAR_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

#if 0
  g_signal_connect (
    G_OBJECT (self->close_notification_button),
    "clicked",
    G_CALLBACK (on_close_notification_clicked),
    NULL);
#endif

  GtkEventControllerKey * key_controller =
    GTK_EVENT_CONTROLLER_KEY (
      gtk_event_controller_key_new ());
  g_signal_connect (
    G_OBJECT (key_controller), "key-pressed",
    G_CALLBACK (on_key_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (key_controller));

  g_signal_connect (
    G_OBJECT (self), "close-request",
    G_CALLBACK (on_close_request), self);

  g_message ("done");
}
