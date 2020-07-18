/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (MainWindowWidget,
               main_window_widget,
               GTK_TYPE_APPLICATION_WINDOW)

static void
on_main_window_destroy (MainWindowWidget * self,
                        gpointer user_data)
{
  /* stop engine and wait to finish the cycle */
  AUDIO_ENGINE->run = 0;
  g_usleep (80);

  if (PROJECT->loaded)
    {
      /* stop events from getting fired. this
       * prevents some segfaults on shutdown */
      event_manager_stop_events (EVENT_MANAGER);

      g_application_quit (
        G_APPLICATION (zrythm_app));
    }
}

static void
on_state_changed (MainWindowWidget * self,
                  GdkEventWindowState  * event,
                  gpointer    user_data)
{
  self->is_maximized =
    event->new_window_state &
      GDK_WINDOW_STATE_MAXIMIZED;
  self->is_fullscreen =
    event->new_window_state &
      GDK_WINDOW_STATE_FULLSCREEN;
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

static void
on_close_notification_clicked (
  GtkButton * btn,
  gpointer    user_data)
{
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    0);
}

/**
 * Returns the selections for the last focused
 * arranger.
 */
ArrangerSelections *
main_window_get_last_focused_arranger_selections (
  MainWindowWidget * self)
{
  ArrangerSelections * sel = NULL;
  if (MAIN_WINDOW_LAST_FOCUSED_IS (
        MW_TIMELINE) ||
      MAIN_WINDOW_LAST_FOCUSED_IS (
        MW_PINNED_TIMELINE))
    {
      sel =
        (ArrangerSelections *) TL_SELECTIONS;
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_MIDI_ARRANGER) ||
           MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_MIDI_MODIFIER_ARRANGER))
    {
      sel =
        (ArrangerSelections *) MA_SELECTIONS;
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_CHORD_ARRANGER))
    {
      sel =
        (ArrangerSelections *) CHORD_SELECTIONS;
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_AUTOMATION_ARRANGER))
    {
      sel =
        (ArrangerSelections *)
        AUTOMATION_SELECTIONS;
    }

  return sel;
}

static gboolean
on_key_release (
  GtkWidget *widget,
  GdkEventKey *event,
  MainWindowWidget * self)
{
  return FALSE;
}

static gboolean
on_key_action (
  GtkWidget *widget,
  GdkEventKey *event,
  ArrangerWidget * self)
{
  g_message ("main window key press");

  if (!z_gtk_keyval_is_arrow (event->keyval))
    return FALSE;

  if (MW_TRACK_INSPECTOR->comment->has_focus)
    {
      return FALSE;
    }

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

  return FALSE;
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

  header_widget_setup (MW_HEADER, PROJECT->title);

  /* setup center dock */
  center_dock_widget_setup (MW_CENTER_DOCK);

  editor_toolbar_widget_setup (
    MW_EDITOR_TOOLBAR);
  timeline_toolbar_widget_setup (
    MW_TIMELINE_TOOLBAR);

  /* setup piano roll */
  if (MW_BOT_DOCK_EDGE && MW_CLIP_EDITOR)
    {
      clip_editor_widget_setup (
        MW_CLIP_EDITOR);
    }

  // set icons
  GtkWidget * image =
    resources_get_icon (
      ICON_TYPE_ZRYTHM, "zrythm.svg");
  gtk_window_set_icon (
    GTK_WINDOW (self),
    gtk_image_get_pixbuf (GTK_IMAGE (image)));

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
    MW_TIMELINE_EVENT_VIEWER);

  EVENTS_PUSH (
    ET_MAIN_WINDOW_LOADED, NULL);

  self->setup = true;

  g_message ("done");
}

/**
 * Prepare for finalization.
 */
void
main_window_widget_tear_down (
  MainWindowWidget * self)
{
  g_message ("tearing down %p...", self);

  self->setup = false;

  if (self->center_dock)
    {
      center_dock_widget_tear_down (
        self->center_dock);
    }

  gtk_window_set_application (
    GTK_WINDOW (self), NULL);

  g_message ("done");
}

static void
main_window_finalize (
  MainWindowWidget * self)
{
  g_message ("finalizing...");

  G_OBJECT_CLASS (
    main_window_widget_parent_class)->
      finalize (G_OBJECT (self));

  g_message ("done");
}

static void
main_window_widget_class_init (
  MainWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "main_window.ui");

  gtk_widget_class_set_css_name (
    klass, "main-window");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, MainWindowWidget, x)

  BIND_CHILD (main_box);
  BIND_CHILD (header);
  BIND_CHILD (top_bar);
  BIND_CHILD (center_box);
  BIND_CHILD (center_dock);
  BIND_CHILD (bot_bar);
  BIND_CHILD (revealer);
  BIND_CHILD (close_notification_button);
  BIND_CHILD (notification_label);
  gtk_widget_class_bind_template_callback (
    klass,
    on_main_window_destroy);
  gtk_widget_class_bind_template_callback (
    klass,
    on_state_changed);

#undef BIND_CHILD

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) main_window_finalize;
}

static void
main_window_widget_init (MainWindowWidget * self)
{
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
    { "snap-to-grid", activate_snap_to_grid },
    { "snap-keep-offset",
      activate_snap_keep_offset },
    { "snap-events", activate_snap_events },
    { "quick-quantize",
      activate_quick_quantize, "s"},
    { "quantize-options", activate_quantize_options,
      "s" },

    /* musical mode */
    { "toggle-musical-mode", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "musical-mode") ?
        "true" : "false",
      change_state_musical_mode },

    /* track actions */
    { "create-audio-track",
      activate_create_audio_track },
    { "create-ins-track",
      activate_create_ins_track },
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
    { "delete-selected-tracks",
      activate_delete_selected_tracks },

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

    /* piano roll */
    { "toggle-drum-mode",
      activate_toggle_drum_mode },
    { "toggle-listen-notes", NULL, NULL,
      g_settings_get_boolean (
        S_UI, "listen-notes") ?
        "true" : "false",
      change_state_listen_notes },

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
  };

  g_action_map_add_action_entries (
    G_ACTION_MAP (self), actions,
    G_N_ELEMENTS (actions), self);

  g_type_ensure (HEADER_WIDGET_TYPE);
  g_type_ensure (TOP_BAR_WIDGET_TYPE);
  g_type_ensure (CENTER_DOCK_WIDGET_TYPE);
  g_type_ensure (BOT_BAR_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->close_notification_button),
    "clicked",
    G_CALLBACK (on_close_notification_clicked),
    NULL);

  g_signal_connect (
    G_OBJECT (self), "key-press-event",
    G_CALLBACK (on_key_action), self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_release), self);
}
