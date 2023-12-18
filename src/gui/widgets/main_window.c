// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "actions/actions.h"
#include "dsp/master_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
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
#include "gui/widgets/header.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
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
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/view_toolbar.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/system.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libpanel.h>

G_DEFINE_TYPE (MainWindowWidget, main_window_widget, ADW_TYPE_APPLICATION_WINDOW)

/**
 * This is called when the window closing is
 * finalized and cannot be intercepted.
 */
static void
on_main_window_destroy (MainWindowWidget * self, gpointer user_data)
{
  g_message ("main window destroy %p", self);

  event_manager_process_now (EVENT_MANAGER);

  /* if this is the current main window and a project
   * is loaded, quit the application (check needed
   * because this is also called right after loading
   * a project) */
  if (PROJECT->loaded && zrythm_app->main_window == self)
    {
      event_manager_stop_events (EVENT_MANAGER);

      object_free_w_func_and_null (project_free, PROJECT);

#if 0
      /* stop engine */
      engine_activate (AUDIO_ENGINE, false);

      /* stop events from getting fired. this
       * prevents some segfaults on shutdown */
      event_manager_stop_events (EVENT_MANAGER);

      /* close any plugin windows */
#endif

      g_application_quit (G_APPLICATION (zrythm_app));
    }

  g_message ("main window destroy called");
}

static void
save_on_quit_response_cb (
  AdwMessageDialog * dialog,
  gchar *            response,
  MainWindowWidget * self)
{
  if (string_is_equal (response, "save-quit"))
    {
      /* save project */
      g_message ("saving project...");
      GError * err = NULL;
      bool     successful = project_save (
        PROJECT, PROJECT->dir, F_NOT_BACKUP, ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC,
        &err);
      if (!successful)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to save project"));
        }
      g_idle_add_once ((GSourceOnceFunc) gtk_window_destroy, self);
    }
  else if (string_is_equal (response, "quit-no-save"))
    {
      g_message ("quitting without saving...");
      g_idle_add_once ((GSourceOnceFunc) gtk_window_destroy, self);
    }
  else if (string_is_equal (response, "cancel"))
    {
      /* no action needed */
    }
}

/**
 * This is called when a close request is handled
 * and can be intercepted.
 */
static bool
on_close_request (GtkWindow * window, MainWindowWidget * self)
{
  g_debug ("%s: main window delete event called", __func__);

  /* ask for save if project has unsaved changes */
  if (project_has_unsaved_changes (PROJECT))
    {
      AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
        window, _ ("Save Changes?"),
        _ ("The project contains unsaved changes. "
           "If you quit without saving, unsaved "
           "changes will be lost.")));
      adw_message_dialog_add_responses (
        dialog, "cancel", _ ("_Cancel"), "quit-no-save", _ ("_Discard"),
        "save-quit", _ ("_Save"), NULL);
      adw_message_dialog_set_close_response (dialog, "cancel");
      adw_message_dialog_set_response_appearance (
        dialog, "quit-no-save", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_response_appearance (
        dialog, "save-quit", ADW_RESPONSE_SUGGESTED);
      adw_message_dialog_set_default_response (dialog, "save-quit");
      g_signal_connect (
        dialog, "response", G_CALLBACK (save_on_quit_response_cb), self);
      gtk_window_present (GTK_WINDOW (dialog));

      /* return true to abort closing main window */
      return true;

    } /* endif project has unsaved changes */

  gtk_window_destroy (GTK_WINDOW (self));
  return false;
}

MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  MainWindowWidget * self = g_object_new (
    MAIN_WINDOW_WIDGET_TYPE, "application", G_APPLICATION (_app), "title",
    PROGRAM_NAME, NULL);

  return self;
}

static gboolean
on_key_pressed (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  gpointer                user_data)
{
  /*g_debug ("main window key press");*/
  MainWindowWidget * self = Z_MAIN_WINDOW_WIDGET (user_data);

  /* if pressed space and currently not inside a GtkEditable,
   * activate the play-pause action */
  if (keyval == GDK_KEY_space || keyval == GDK_KEY_KP_Space)
    {
      g_debug ("space pressed");
      GtkWidget * focus_child = gtk_widget_get_focus_child (GTK_WIDGET (self));
      GtkWidget * next_child = focus_child;
      while (next_child)
        {
          focus_child = next_child;
          next_child = gtk_widget_get_focus_child (GTK_WIDGET (focus_child));
        }
      z_gtk_widget_print_hierarchy (focus_child);
      if (!GTK_IS_EDITABLE (focus_child))
        {
          g_action_group_activate_action (
            G_ACTION_GROUP (zrythm_app), "play-pause", NULL);
          return true;
        }
    }

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
      return true;
    }
#endif

  return false;
}

static bool
show_startup_errors (MainWindowWidget * self)
{
  /* show any startup errors */
  for (int k = 0; k < zrythm_app->num_startup_errors; k++)
    {
      char * msg = zrythm_app->startup_errors[k];
      ui_show_error_message (_ ("Startup Error"), msg);
      g_free (msg);
    }
  zrythm_app->num_startup_errors = 0;

  return G_SOURCE_REMOVE;
}

void
main_window_widget_setup (MainWindowWidget * self)
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

  editor_toolbar_widget_setup (MW_EDITOR_TOOLBAR);

  /* setup piano roll */
  if (MW_BOT_DOCK_EDGE && MW_CLIP_EDITOR)
    {
      clip_editor_widget_setup (MW_CLIP_EDITOR);
    }

  // set icons
  /*gtk_window_set_icon_name (*/
  /*GTK_WINDOW (self), "zrythm");*/

  /* setup top and bot bars */
  top_bar_widget_refresh (self->top_bar);
  bot_bar_widget_setup (self->bot_bar);

  /* setup mixer */
  g_warn_if_fail (P_MASTER_TRACK && P_MASTER_TRACK->channel);
  mixer_widget_setup (MW_MIXER, P_MASTER_TRACK->channel);

  gtk_window_maximize (GTK_WINDOW (self));

  /* show track selection info */
  g_warn_if_fail (TRACKLIST_SELECTIONS->tracks[0]);
  EVENTS_PUSH (ET_TRACK_CHANGED, TRACKLIST_SELECTIONS->tracks[0]);
  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, TL_SELECTIONS);
  event_viewer_widget_refresh (MW_TIMELINE_EVENT_VIEWER, false);

  EVENTS_PUSH (ET_MAIN_WINDOW_LOADED, NULL);

  g_idle_add ((GSourceFunc) show_startup_errors, self);

  gtk_window_present (GTK_WINDOW (self));

  self->setup = true;

  event_manager_process_now (EVENT_MANAGER);

  g_message ("done");
}

void
main_window_widget_set_project_title (MainWindowWidget * self, Project * prj)
{
  char * prj_title;
  if (project_has_unsaved_changes (prj))
    {
      prj_title = g_strdup_printf ("%s*", prj->title);
    }
  else
    {
      prj_title = g_strdup (prj->title);
    }
  adw_window_title_set_title (self->window_title, prj_title);
  adw_window_title_set_subtitle (self->window_title, prj->dir);
  g_free (prj_title);
}

static void
on_focus_widget_changed (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  /* below is for debugging */
  GtkWidget * focus_widget = gtk_window_get_focus (GTK_WINDOW (gobject));
  if (focus_widget)
    {
      /*z_gtk_widget_print_hierarchy (focus_widget);*/
    }
  else
    {
      g_debug ("nothing focused");
    }
}

/**
 * Prepare for finalization.
 * FIXME do this logic inside dispose() or finalize() (check)
 */
void
main_window_widget_tear_down (MainWindowWidget * self)
{
  g_message ("tearing down main window %p...", self);

  self->setup = false;

  if (self->center_dock)
    {
      center_dock_widget_tear_down (self->center_dock);
    }

  gtk_window_set_application (GTK_WINDOW (self), NULL);

  g_message ("done tearing down main window");
}

static void
main_window_dispose (MainWindowWidget * self)
{
  g_message ("disposing main_window...");

  G_OBJECT_CLASS (main_window_widget_parent_class)->dispose (G_OBJECT (self));

  g_message ("done");
}

static void
main_window_finalize (MainWindowWidget * self)
{
  g_message ("finalizing main_window...");

  G_OBJECT_CLASS (main_window_widget_parent_class)->finalize (G_OBJECT (self));

  g_message ("done");
}

static void
main_window_widget_class_init (MainWindowWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  resources_set_class_template (wklass, "main_window.ui");
  gtk_widget_class_set_css_name (wklass, "main-window");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (wklass, MainWindowWidget, x)

  BIND_CHILD (toast_overlay);
  BIND_CHILD (main_box);
  BIND_CHILD (header_bar);
  BIND_CHILD (start_dock_switcher);
  BIND_CHILD (end_dock_switcher);
  BIND_CHILD (window_title);
  BIND_CHILD (view_switcher);
  BIND_CHILD (preferences);
  BIND_CHILD (log_viewer);
  BIND_CHILD (scripting_interface);
  BIND_CHILD (header);
  BIND_CHILD (top_bar);
  BIND_CHILD (center_box);
  BIND_CHILD (center_dock);
  BIND_CHILD (bot_bar);

  gtk_widget_class_bind_template_callback (klass, on_main_window_destroy);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) main_window_dispose;
  oklass->finalize = (GObjectFinalizeFunc) main_window_finalize;
}

static void
main_window_widget_init (MainWindowWidget * self)
{
  g_message ("Initing main window widget...");

  GActionEntry actions[] = {

  /* global shortcuts */
    { "cycle-focus", activate_cycle_focus },
    { "cycle-focus-backwards", activate_cycle_focus_backwards },
    { "focus-first-widget", activate_focus_first_widget },

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
    { "mute-selection", activate_mute_selection, "s" },

 /* view menu */
    { "toggle-left-panel", activate_toggle_left_panel },
    { "toggle-right-panel", activate_toggle_right_panel },
    { "toggle-bot-panel", activate_toggle_bot_panel },
    { "toggle-top-panel", activate_toggle_top_panel },
    { "toggle-status-bar", activate_toggle_status_bar },
    { "zoom-in", activate_zoom_in, "s" },
    { "zoom-out", activate_zoom_out, "s" },
    { "original-size", activate_original_size, "s" },
    { "best-fit", activate_best_fit, "s" },

 /* snapping, quantize */
    { "snap-to-grid", activate_snap_to_grid, "s" },
    { "snap-keep-offset", activate_snap_keep_offset, "s" },
    { "snap-events", activate_snap_events, "s" },
    { "quick-quantize", activate_quick_quantize, "s" },
    { "quantize-options", activate_quantize_options, "s" },

 /* range actions */
    { "insert-silence", activate_insert_silence },
    { "remove-range", activate_remove_range },

 /* playhead actions */
    { "timeline-playhead-scroll-edges", NULL, NULL,
     g_settings_get_boolean (S_UI, "timeline-playhead-scroll-edges")
        ? "true"
        : "false",
     change_state_timeline_playhead_scroll_edges },
    { "timeline-playhead-follow", NULL, NULL,
     g_settings_get_boolean (S_UI, "timeline-playhead-follow") ? "true" : "false",
     change_state_timeline_playhead_follow },
    { "editor-playhead-scroll-edges", NULL, NULL,
     g_settings_get_boolean (S_UI, "editor-playhead-scroll-edges")
        ? "true"
        : "false",
     change_state_editor_playhead_scroll_edges },
    { "editor-playhead-follow", NULL, NULL,
     g_settings_get_boolean (S_UI, "editor-playhead-follow") ? "true" : "false",
     change_state_editor_playhead_follow },

 /* merge actions */
    { "merge-selection", activate_merge_selection },

 /* musical mode */
    { "toggle-musical-mode", NULL, NULL,
     g_settings_get_boolean (S_UI, "musical-mode") ? "true" : "false",
     change_state_musical_mode },

 /* track actions */
    { "create-audio-track", activate_create_audio_track },
    { "create-audio-bus-track", activate_create_audio_bus_track },
    { "create-midi-bus-track", activate_create_midi_bus_track },
    { "create-midi-track", activate_create_midi_track },
    { "create-audio-group-track", activate_create_audio_group_track },
    { "create-midi-group-track", activate_create_midi_group_track },
    { "create-folder-track", activate_create_folder_track },
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
     g_settings_get_boolean (S_TRANSPORT, "metronome-enabled") ? "true" : "false",
     change_state_metronome },
    { "toggle-loop", NULL, NULL,
     g_settings_get_boolean (S_TRANSPORT, "loop") ? "true" : "false",
     change_state_loop },
    { "goto-start-marker", activate_goto_start_marker },
    { "goto-end-marker", activate_goto_end_marker },
    { "goto-prev-marker", activate_goto_prev_marker },
    { "goto-next-marker", activate_goto_next_marker },
    { "play-pause", activate_play_pause },
    { "record-play", activate_record_play },
    { "go-to-start", activate_go_to_start },
    { "input-bpm", activate_input_bpm },
    { "tap-bpm", activate_tap_bpm },

 /* transport - jack */
    { "set-timebase-master", activate_set_timebase_master },
    { "set-transport-client", activate_set_transport_client },
    { "unlink-jack-transport", activate_unlink_jack_transport },

 /* tracks */
    { "delete-selected-tracks", activate_delete_selected_tracks },
    { "duplicate-selected-tracks", activate_duplicate_selected_tracks },
    { "hide-selected-tracks", activate_hide_selected_tracks },
    { "pin-selected-tracks", activate_pin_selected_tracks },
    { "solo-selected-tracks", activate_solo_selected_tracks },
    { "unsolo-selected-tracks", activate_unsolo_selected_tracks },
    { "mute-selected-tracks", activate_mute_selected_tracks },
    { "unmute-selected-tracks", activate_unmute_selected_tracks },
    { "listen-selected-tracks", activate_listen_selected_tracks },
    { "unlisten-selected-tracks", activate_unlisten_selected_tracks },
    { "enable-selected-tracks", activate_enable_selected_tracks },
    { "disable-selected-tracks", activate_disable_selected_tracks },
    { "change-track-color", activate_change_track_color },
    { "track-set-midi-channel", activate_track_set_midi_channel, "s" },
    { "quick-bounce-selected-tracks", activate_quick_bounce_selected_tracks },
    { "bounce-selected-tracks", activate_bounce_selected_tracks },
    { "selected-tracks-direct-out-to", activate_selected_tracks_direct_out_to,
     "i" },
    {
     "selected-tracks-direct-out-new", activate_selected_tracks_direct_out_new,
     },
    {
     "toggle-track-passthrough-input", activate_toggle_track_passthrough_input,
     },
    {
     "show-used-automation-lanes-on-selected-tracks", activate_show_used_automation_lanes_on_selected_tracks,
     },
    {
     "hide-unused-automation-lanes-on-selected-tracks", activate_hide_unused_automation_lanes_on_selected_tracks,
     },
    { "append-track-objects-to-selection",
     activate_append_track_objects_to_selection, "i" },
    { "append-lane-objects-to-selection",
     activate_append_lane_objects_to_selection, "(ii)" },
    { "append-lane-automation-regions-to-selection",
     activate_append_lane_automation_regions_to_selection, "(ii)" },

 /* piano roll */
    { "toggle-drum-mode", activate_toggle_drum_mode },
    { "toggle-listen-notes", NULL, NULL,
     g_settings_get_boolean (S_UI, "listen-notes") ? "true" : "false",
     change_state_listen_notes },
    { "midi-editor.highlighting", activate_midi_editor_highlighting, "s" },
    { "toggle-ghost-notes", NULL, NULL,
     g_settings_get_boolean (S_UI, "ghost-notes") ? "true" : "false",
     change_state_ghost_notes },

 /* automation */
    { "show-automation-values", NULL, NULL,
     g_settings_get_boolean (S_UI, "show-automation-values") ? "true" : "false",
     change_state_show_automation_values },

 /* control room */
    { "toggle-dim-output", NULL, NULL, "true", change_state_dim_output },

 /* show/hide event viewers */
    { "toggle-timeline-event-viewer", activate_toggle_timeline_event_viewer },
    { "toggle-editor-event-viewer", activate_toggle_editor_event_viewer },

 /* editor functions */
    { "editor-function", activate_editor_function, "s" },
    { "editor-function-lv2", activate_editor_function_lv2, "s" },

 /* rename track/region */
    { "rename-track", activate_rename_track },
    { "rename-arranger-object", activate_rename_arranger_object },

 /* arranger selections */
    { "nudge-selection", activate_nudge_selection, "s" },
    { "detect-bpm", activate_detect_bpm, "s" },
    { "timeline-function", activate_timeline_function, "i" },
    { "quick-bounce-selections", activate_quick_bounce_selections },
    { "bounce-selections", activate_bounce_selections },
    {
     "set-curve-algorithm", activate_set_curve_algorithm,
     "i", },
    {
     "set-region-fade-in-algorithm-preset", activate_set_region_fade_in_algorithm_preset,
     "s", },
    {
     "set-region-fade-out-algorithm-preset", activate_set_region_fade_out_algorithm_preset,
     "s", },
    { "arranger-object-view-info", activate_arranger_object_view_info, "s" },
    { "export-midi-regions", activate_export_midi_regions },
    { "create-arranger-obj", activate_create_arranger_object, "(sdd)" },
    { "change-region-color", activate_change_region_color },
    { "reset-region-color", activate_reset_region_color },
    { "move-automation-regions", activate_move_automation_regions, "s" },

 /* chord presets */
    {
     "save-chord-preset", activate_save_chord_preset,
     },
    { "load-chord-preset", activate_load_chord_preset, "s" },
    { "load-chord-preset-from-scale", activate_load_chord_preset_from_scale,
     "s" },
    { "transpose-chord-pad", activate_transpose_chord_pad, "s" },
    {
     "add-chord-preset-pack", activate_add_chord_preset_pack,
     },
    { "delete-chord-preset-pack", activate_delete_chord_preset_pack, "s" },
    { "rename-chord-preset-pack", activate_rename_chord_preset_pack, "s" },
    { "delete-chord-preset", activate_delete_chord_preset, "s" },
    { "rename-chord-preset", activate_rename_chord_preset, "s" },

 /* cc bindings */
    { "bind-midi-cc", activate_bind_midi_cc, "s" },
    { "delete-cc-binding", activate_delete_cc_binding, "i" },

 /* port actions */
    { "reset-stereo-balance", activate_reset_stereo_balance, "s" },
    { "reset-fader", activate_reset_fader, "s" },
    { "reset-control", activate_reset_control, "s" },
    { "port-view-info", activate_port_view_info, "s" },
    { "port-connection-remove", activate_port_connection_remove },

 /* plugin actions */
    { "plugin-toggle-enabled", activate_plugin_toggle_enabled, "s" },
    { "plugin-inspect", activate_plugin_inspect },
    { "mixer-selections-delete", activate_mixer_selections_delete },

 /* panel file browser actions */
    { "panel-file-browser-add-bookmark",
     activate_panel_file_browser_add_bookmark, "s" },
    { "panel-file-browser-delete-bookmark",
     activate_panel_file_browser_delete_bookmark },

 /* pluginbrowser actions */
    { "plugin-browser-add-to-project", activate_plugin_browser_add_to_project,
     "s" },
    { "plugin-browser-add-to-project-carla",
     activate_plugin_browser_add_to_project_carla, "s" },
    { "plugin-browser-add-to-project-bridged-ui",
     activate_plugin_browser_add_to_project_bridged_ui, "s" },
    { "plugin-browser-add-to-project-bridged-full",
     activate_plugin_browser_add_to_project_bridged_full, "s" },
    { "plugin-browser-toggle-generic-ui", NULL, NULL, "false",
     change_state_plugin_browser_toggle_generic_ui },
    { "plugin-browser-add-to-collection",
     activate_plugin_browser_add_to_collection, "s" },
    { "plugin-browser-remove-from-collection",
     activate_plugin_browser_remove_from_collection, "s" },
    { "plugin-browser-reset", activate_plugin_browser_reset, "s" },
    {
     "plugin-collection-add", activate_plugin_collection_add,
     },
    {
     "plugin-collection-rename", activate_plugin_collection_rename,
     },
    {
     "plugin-collection-remove", activate_plugin_collection_remove,
     },
  };

#if 0
  g_action_map_add_action_entries (
    G_ACTION_MAP (self), actions,
    G_N_ELEMENTS (actions), self);
#endif
  g_action_map_add_action_entries (
    G_ACTION_MAP (zrythm_app), actions, G_N_ELEMENTS (actions), zrythm_app);

  g_type_ensure (HEADER_WIDGET_TYPE);
  g_type_ensure (TOP_BAR_WIDGET_TYPE);
  g_type_ensure (CENTER_DOCK_WIDGET_TYPE);
  g_type_ensure (BOT_BAR_WIDGET_TYPE);
  g_type_ensure (PANEL_TYPE_TOGGLE_BUTTON);

  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_DOCK(child) \
  g_object_set (G_OBJECT (child), "dock", self->center_dock->dock, NULL)

  SET_DOCK (self->start_dock_switcher);
  SET_DOCK (self->end_dock_switcher);
  SET_DOCK (self->bot_bar->bot_dock_switcher);
  SET_DOCK (self->header->view_toolbar->left_panel);
  SET_DOCK (self->header->view_toolbar->right_panel);
  SET_DOCK (self->header->view_toolbar->bot_panel);
  SET_DOCK (self->header->view_toolbar->top_panel);

#undef SET_DOCK

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable (GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (preferences, _ ("Preferences"));
  SET_TOOLTIP (log_viewer, _ ("Log viewer"));
  SET_TOOLTIP (scripting_interface, _ ("Scripting interface"));
#undef SET_TOOLTIP

  adw_view_switcher_set_stack (self->view_switcher, self->header->stack);
  adw_view_switcher_set_policy (
    self->view_switcher, ADW_VIEW_SWITCHER_POLICY_WIDE);

  /* make sure the header bar includes the app
   * icon (if not, add it at the start) */
  gchar * strval;
  g_object_get (
    G_OBJECT (zrythm_app->default_settings), "gtk-decoration-layout", &strval,
    NULL);
  if (!string_contains_substr (strval, "icon"))
    {
      char * new_layout = new_layout = g_strdup_printf ("icon,%s", strval);
      gtk_header_bar_set_decoration_layout (self->header_bar, new_layout);
      g_free (new_layout);
    }
  g_free (strval);

  GtkEventControllerKey * key_controller =
    GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (key_controller), GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (key_controller), "key-pressed", G_CALLBACK (on_key_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (key_controller));

  g_signal_connect (
    G_OBJECT (self), "close-request", G_CALLBACK (on_close_request), self);

  g_signal_connect (
    G_OBJECT (self), "notify::focus-widget",
    G_CALLBACK (on_focus_widget_changed), self);

  g_message ("done");
}
