/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * GAction actions.
 */

#include "config.h"

#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "actions/delete_timeline_selections_action.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/preferences.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/top_dock_edge.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/dialogs.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

void
action_enable_window_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (MAIN_WINDOW),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    1);
}

void
action_disable_window_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (MAIN_WINDOW),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    0);
}

void
action_enable_app_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (ZRYTHM),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    1);
}

void
action_disable_app_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (ZRYTHM),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    0);
}

void
activate_about (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  gtk_show_about_dialog (
    GTK_WINDOW (MAIN_WINDOW),
    "copyright", "Copyright (C) 2019 Alexandros Theodotou",
    "logo-icon-name", "z",
    "program-name", "Zrythm",
    "comments", "An highly automated, intuitive, Digital Audio Workstation (DAW)",
    "license-type", GTK_LICENSE_GPL_3_0,
    "website", "https://www.zrythm.org",
    "website-label", "Official Website",
    "version", "v" PACKAGE_VERSION,
    NULL);
}

void
activate_quit (GSimpleAction *action,
               GVariant      *variant,
               gpointer       user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

void
activate_shortcuts (GSimpleAction *action,
                    GVariant      *variant,
                    gpointer       user_data)
{
}

/**
 * Show preferences window.
 */
void
activate_preferences (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  PreferencesWidget * widget =
    preferences_widget_new ();
  gtk_widget_set_visible (GTK_WIDGET (widget),
                          1);
}

/**
 * Activate audition mode.
 */
void
activate_audition_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_AUDITION;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate select mode.
 */
void
activate_select_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  if (P_TOOL == TOOL_SELECT_NORMAL)
    P_TOOL = TOOL_SELECT_STRETCH;
  else
    P_TOOL = TOOL_SELECT_NORMAL;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate edit mode.
 */
void
activate_edit_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_EDIT;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate eraser mode.
 */
void
activate_eraser_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_ERASER;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate ramp mode.
 */
void
activate_ramp_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_RAMP;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/* FIXME rename to timeline zoom in */
void
activate_zoom_in (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  RULER_WIDGET_GET_PRIVATE (
    Z_RULER_WIDGET (MW_RULER));
  ruler_widget_set_zoom_level (
    Z_RULER_WIDGET (MW_RULER),
    rw_prv->zoom_level * 1.3f);

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL)
}

void
activate_zoom_out (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  RULER_WIDGET_GET_PRIVATE (
    Z_RULER_WIDGET (MW_RULER));
  double zoom_level = rw_prv->zoom_level / 1.3;
  ruler_widget_set_zoom_level (
    Z_RULER_WIDGET (MW_RULER),
    zoom_level);

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL)
}

void
activate_best_fit (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  /* TODO */
  g_message ("ZOOMING IN");

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL)
}

void
activate_original_size (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  RULER_WIDGET_GET_PRIVATE (
    Z_RULER_WIDGET (MW_RULER));
  rw_prv->zoom_level = DEFAULT_ZOOM_LEVEL;

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL)
}

void
activate_loop_selection (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->last_focused ==
      GTK_WIDGET (MW_TIMELINE))
    {
      if (!timeline_selections_has_any (
            TL_SELECTIONS))
        return;

      Position start, end;
      timeline_selections_get_start_pos (
        TL_SELECTIONS, &start);
      timeline_selections_get_end_pos (
        TL_SELECTIONS, &end);

      position_set_to_pos (
        &TRANSPORT->loop_start_pos,
        &start);
      position_set_to_pos (
        &TRANSPORT->loop_end_pos,
        &end);

      transport_update_position_frames ();
      EVENTS_PUSH (
        ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
        NULL);
    }
}

void
activate_new (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

int
run_open_dialog (GtkDialog * dialog)
{
  int res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser =
        GTK_FILE_CHOOSER (dialog);
      char * filename =
        gtk_file_chooser_get_filename (chooser);
      g_message ("filename %s", filename);
      res = project_load (filename);
      g_free (filename);
    }

  return res;
}

void
activate_open (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  GtkDialog * dialog =
    dialogs_get_open_project_dialog (
      GTK_WINDOW (MAIN_WINDOW));

  int res;
  do
    res = run_open_dialog (dialog);
  while (res != 0);

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
activate_save (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
   if (!PROJECT->dir ||
       !PROJECT->title)
     {
       activate_save_as (action,
                         variant,
                         user_data);
       return;
     }
   g_message ("%s project dir ", PROJECT->dir);

   project_save (PROJECT->dir);
}

void
activate_save_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction _action = GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  dialog = gtk_file_chooser_dialog_new ("Save Project",
                                        GTK_WINDOW (MAIN_WINDOW),
                                        _action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Save",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  chooser = GTK_FILE_CHOOSER (dialog);

  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  if (PROJECT->title)
    gtk_file_chooser_set_current_name (chooser,
                                       PROJECT->title);
  else
    gtk_file_chooser_set_filename (chooser,
                                   "Untitled project");

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename;

      filename = gtk_file_chooser_get_filename (chooser);
      project_save (filename);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

void
activate_iconify (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  gtk_window_iconify (GTK_WINDOW (MAIN_WINDOW));
}

void
activate_export_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  /* if project is loaded */
  if (PROJECT->project_file_path)
    {
      ExportDialogWidget * export = export_dialog_widget_new ();
      gtk_dialog_run (GTK_DIALOG (export));
    }
  else
    {
      ui_show_error_message (
        MAIN_WINDOW,
        "Project doesn't have a path yet. Please"
        " save the project before exporting.");
    }
}

void
activate_properties (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_undo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (stack_is_empty (UNDO_MANAGER->undo_stack))
    return;
  undo_manager_undo (UNDO_MANAGER);

  EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE,
               NULL);
}

void
activate_redo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (stack_is_empty (UNDO_MANAGER->redo_stack))
    return;
  undo_manager_redo (UNDO_MANAGER);

  EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE,
               NULL);
}

void
activate_cut (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}


void
activate_copy (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->last_focused == GTK_WIDGET (MW_TIMELINE))
    {
      gtk_clipboard_set_text (
        DEFAULT_CLIPBOARD,
        timeline_selections_serialize (
          TL_SELECTIONS),
        -1);
    }
}

static void
on_timeline_clipboard_received (
  GtkClipboard *     clipboard,
  const char *       text,
  gpointer           data)
{
  g_message ("clipboard data received: %s",
             text);

  if (!text)
    return;
  TimelineSelections * ts =
    timeline_selections_deserialize (text);
  g_message ("printing deserialized");
  timeline_selections_print (ts);

  if (MAIN_WINDOW->last_focused ==
      GTK_WIDGET (MW_TIMELINE))
    {
      timeline_selections_paste_to_pos (ts,
                                        &PLAYHEAD);
    }

  /* free ts */
  g_message ("paste done");
}

void
activate_paste (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("paste");
  gtk_clipboard_request_text (
    DEFAULT_CLIPBOARD,
    on_timeline_clipboard_received,
    NULL);
}

void
activate_delete (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->last_focused == GTK_WIDGET (MW_TIMELINE))
    {
      UndoableAction * action =
        delete_timeline_selections_action_new ();
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
}

void
activate_clear_selection (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_select_all (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_toggle_left_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  CenterDockWidget * self = MW_CENTER_DOCK;
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (self),
                         "left-visible",
                         &a);
  int val = g_value_get_boolean (&a);
  g_value_set_boolean (&a,
                       val == 1 ? 0 : 1);
  g_object_set_property (G_OBJECT (self),
                         "left-visible",
                         &a);

  /* TODO update header bar buttons */
}

void
activate_toggle_right_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  CenterDockWidget * self = MW_CENTER_DOCK;
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (self),
                         "right-visible",
                         &a);
  int val = g_value_get_boolean (&a);
  g_value_set_boolean (&a,
                       val == 1 ? 0 : 1);
  g_object_set_property (G_OBJECT (self),
                         "right-visible",
                         &a);
}

void
activate_toggle_bot_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  CenterDockWidget * self = MW_CENTER_DOCK;
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_object_get_property (G_OBJECT (self),
                         "bottom-visible",
                         &a);
  int val = g_value_get_boolean (&a);
  g_value_set_boolean (&a,
                       val == 1 ? 0 : 1);
  g_object_set_property (G_OBJECT (self),
                         "bottom-visible",
                         &a);
}

void
activate_toggle_status_bar (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_fullscreen (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->is_maximized)
    {
      gtk_window_unmaximize (
        GTK_WINDOW (MAIN_WINDOW));
    }
  else
    {
      gtk_window_maximize (
        GTK_WINDOW (MAIN_WINDOW));
    }
}

void
activate_manual (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
}

void
activate_snap_to_grid (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  SNAP_GRID_TIMELINE->snap_to_grid =
    !SNAP_GRID_TIMELINE->snap_to_grid;
}

void
activate_snap_keep_offset (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  SNAP_GRID_TIMELINE->snap_to_grid_keep_offset =
    !SNAP_GRID_TIMELINE->snap_to_grid_keep_offset;
}

void
activate_snap_events (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  SNAP_GRID_TIMELINE->snap_to_events =
    !SNAP_GRID_TIMELINE->snap_to_events;
}

void
activate_create_audio_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  Channel * chan =
    channel_create (CT_AUDIO, "Audio Track");
  mixer_add_channel (chan);
  tracklist_append_track (chan->track);

  EVENTS_PUSH (ET_TRACK_ADDED, chan->track);
}

void
activate_create_ins_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  Channel * chan =
    channel_create (CT_MIDI, "Instrument Track");
  mixer_add_channel (chan);
  tracklist_append_track (chan->track);

  EVENTS_PUSH (ET_TRACK_ADDED, chan->track);
}

void
activate_create_bus_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  Channel * chan =
    channel_create (CT_BUS, "Bus Track");
  mixer_add_channel (chan);
  tracklist_append_track (chan->track);

  EVENTS_PUSH (ET_TRACK_ADDED, chan->track);
}

void
activate_delete_selected_tracks (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  GET_SELECTED_TRACKS;

  for (int i = 0; i < num_selected; i++)
    {
      Track * track = selected_tracks[i];
      switch (track->type)
        {
        case TRACK_TYPE_CHORD:
          break;
        case TRACK_TYPE_INSTRUMENT:
        case TRACK_TYPE_MASTER:
        case TRACK_TYPE_BUS:
        case TRACK_TYPE_AUDIO:
            {
              ChannelTrack * ct = (ChannelTrack *) track;
              mixer_remove_channel (ct->channel);
              tracklist_remove_track (track);
              break;
            }
        }
    }
}
