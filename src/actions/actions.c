/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "actions/create_tracks_action.h"
#include "actions/delete_tracks_action.h"
#include "actions/delete_midi_arranger_selections_action.h"
#include "actions/delete_timeline_selections_action.h"
#include "actions/duplicate_midi_arranger_selections_action.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/donate_dialog.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/preferences.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/dialogs.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

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
activate_manual (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
#ifdef _WIN32
  ShellExecute (
    0, (LPCSTR)"open",
    (LPCSTR) "https://manual.zrythm.org",
    0, 0, SW_SHOWNORMAL);
#else
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://manual.zrythm.org",
    0,
    NULL);
#endif
}

void
activate_chat (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
#ifdef _WIN32
  ShellExecute (
    0, (LPCSTR)"open",
    (LPCSTR) "https://riot.im/app/#/room/#freenode_#zrythm:matrix.org",
    0, 0, SW_SHOWNORMAL);
#else
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://riot.im/app/#/room/#freenode_#zrythm:matrix.org",
    0,
    NULL);
#endif
}

void
activate_forums (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
#ifdef _WIN32
  ShellExecute (
    0, (LPCSTR)"open",
    (LPCSTR) "https://forum.zrythm.org",
    0, 0, SW_SHOWNORMAL);
#else
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://forum.zrythm.org",
    0,
    NULL);
#endif
}

void
activate_donate (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  DonateDialogWidget * donate =
    donate_dialog_widget_new ();
  gtk_window_set_transient_for (
    GTK_WINDOW (donate),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_dialog_run (GTK_DIALOG (donate));
  gtk_widget_destroy (GTK_WIDGET (donate));
}

void
activate_bugreport (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
#ifdef _WIN32
  ShellExecute (
    0, (LPCSTR)"open",
    (LPCSTR) NEW_ISSUE_URL,
    0, 0, SW_SHOWNORMAL);
#else
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    NEW_ISSUE_URL,
    0,
    NULL);
#endif
}

void
activate_about (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  GtkImage * img =
    GTK_IMAGE (
      resources_get_icon (ICON_TYPE_ZRYTHM,
                          "z-splash.svg"));

  char * artists[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      NULL
    };
  char * authors[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      "Sascha Bast <sash@mischkonsum.org>",
      "Georg Krause",
      NULL
    };
  char * documenters[] =
    {
      "Alexandros Theodotou <alex@zrythm.org>",
      NULL
    };
  char * translators =
      "Alexandros Theodotou <alex@zrythm.org>\n"
      "Nicolas Faure <sub26nico@laposte.net>\n"
      "Waui <wau-wau@tutanota.de>\n"
      "Allan Nordhøy <epost@anotheragency.no>\n"
      "WaldiS <admin@sto.ugu.pl>";

  gtk_show_about_dialog (
    GTK_WINDOW (MAIN_WINDOW),
    "artists", artists,
    "authors", authors,
    "copyright", "Copyright (C) 2018-2019 Alexandros Theodotou",
    "documenters", documenters,
    /*"logo-icon-name", "z",*/
    "logo", gtk_image_get_pixbuf (img),
    "program-name", "Zrythm",
    "comments", "An highly automated, intuitive, Digital Audio Workstation (DAW)",
    "license-type", GTK_LICENSE_AGPL_3_0,
    "translator-credits", translators,
    "website", "https://www.zrythm.org",
    "website-label", "Home Page",
    "version", "v" PACKAGE_VERSION,
    NULL);
  gtk_widget_destroy (GTK_WIDGET (img));
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
  GtkBuilder *builder;
  GtkWidget *overlay;

  builder =
    gtk_builder_new_from_resource (
      RESOURCE_PATH TEMPLATE_PATH "shortcuts.ui");
  overlay =
    GTK_WIDGET (
      gtk_builder_get_object (builder, "shortcuts-builder"));
  gtk_window_set_transient_for (
    GTK_WINDOW (overlay),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_widget_show (overlay);
  g_object_unref (builder);
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
  gtk_window_set_transient_for (
    GTK_WINDOW (widget),
    GTK_WINDOW (MAIN_WINDOW));
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
 * Activate cut mode.
 */
void
activate_cut_mode (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  P_TOOL = TOOL_CUT;
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
               NULL);
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
               NULL);
}

void
activate_best_fit (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  /* TODO */
  g_message ("ZOOMING IN");

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL);
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
               NULL);
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
        TL_SELECTIONS, &start, F_NO_TRANSIENTS);
      timeline_selections_get_end_pos (
        TL_SELECTIONS, &end, F_NO_TRANSIENTS);

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

  int res = run_open_dialog (dialog);
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename =
        gtk_file_chooser_get_filename (
          GTK_FILE_CHOOSER (dialog));
      project_load (filename);
    }

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
   g_message ("%s project dir", PROJECT->dir);

   char * dir = g_strdup (PROJECT->dir);
   project_save (dir);
   g_free (dir);
}

void
activate_save_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction _action =
    GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  dialog =
    gtk_file_chooser_dialog_new (
      _("Save Project"),
      GTK_WINDOW (MAIN_WINDOW),
      _action,
      _("_Cancel"),
      GTK_RESPONSE_CANCEL,
      _("_Save"),
      GTK_RESPONSE_ACCEPT,
      NULL);
  chooser = GTK_FILE_CHOOSER (dialog);

  gtk_file_chooser_set_do_overwrite_confirmation (
    chooser, TRUE);

  char * str =
    io_path_get_parent_dir (
      PROJECT->project_file_path);
  gtk_file_chooser_select_filename (
    chooser,
    str);
  g_free (str);
  gtk_file_chooser_set_current_name (
    chooser, PROJECT->title);

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename;

      filename =
        gtk_file_chooser_get_filename (chooser);
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
      ExportDialogWidget * export =
        export_dialog_widget_new ();
      gtk_window_set_transient_for (
        GTK_WINDOW (export),
        GTK_WINDOW (MAIN_WINDOW));
      gtk_dialog_run (GTK_DIALOG (export));
      gtk_widget_destroy (GTK_WIDGET (export));
    }
  else
    {
      ui_show_error_message (
        MAIN_WINDOW,
        _("Project doesn't have a path yet. Please \
save the project before exporting."));
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
  if (MAIN_WINDOW->last_focused ==
        GTK_WIDGET (MW_TIMELINE))
    {
      UndoableAction * action =
        delete_timeline_selections_action_new (
          TL_SELECTIONS);
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else if (MAIN_WINDOW->last_focused ==
             GTK_WIDGET (MW_MIDI_ARRANGER))
    {
      UndoableAction * action =
        delete_midi_arranger_selections_action_new (
          MA_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, action);
    }
}

void
activate_duplicate (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->last_focused ==
        GTK_WIDGET (MW_TIMELINE))
    {
    }
  else if (MAIN_WINDOW->last_focused ==
             GTK_WIDGET (MW_MIDI_ARRANGER))
    {
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
activate_select_all (
	GSimpleAction *action,
	GVariant *variant,
	gpointer user_data)
{
	if (MAIN_WINDOW->last_focused
		== GTK_WIDGET (MW_MIDI_ARRANGER))
	{
		midi_arranger_widget_select_all (
			Z_MIDI_ARRANGER_WIDGET (
				MAIN_WINDOW->last_focused),
			1);
	}
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
  gtk_widget_set_visible (
    GTK_WIDGET (MW_BOT_DOCK_EDGE),
    !gtk_widget_get_visible (
      GTK_WIDGET (MW_BOT_DOCK_EDGE)));
}

/**
 * Toggle timeline visibility.
 */
void
activate_toggle_top_panel (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  gtk_widget_set_visible (
    GTK_WIDGET (MW_CENTER_DOCK->editor_top),
    !gtk_widget_get_visible (
      GTK_WIDGET (MW_CENTER_DOCK->editor_top)));
}

void
activate_toggle_status_bar (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  gtk_widget_set_visible (
    GTK_WIDGET (MW_BOT_BAR),
    !gtk_widget_get_visible (
      GTK_WIDGET (MW_BOT_BAR)));
}

/**
 * Toggle drum mode in the piano roll.
 */
void
activate_toggle_drum_mode (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  PIANO_ROLL->drum_mode = !PIANO_ROLL->drum_mode;

  EVENTS_PUSH (ET_DRUM_MODE_CHANGED, NULL);
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
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO,
      NULL,
      NULL,
      TRACKLIST->num_tracks,
      1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_ins_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT,
      NULL,
      NULL,
      TRACKLIST->num_tracks,
      1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_bus_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_BUS,
      NULL,
      NULL,
      TRACKLIST->num_tracks,
      1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_group_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_GROUP,
      NULL,
      NULL,
      TRACKLIST->num_tracks,
      1);

  undo_manager_perform (UNDO_MANAGER, ua);
}


void
activate_duplicate_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("duplicating selected tracks");

}

void
activate_goto_prev_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  transport_goto_prev_marker (
    TRANSPORT);
}

void
activate_goto_next_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  transport_goto_next_marker (
    TRANSPORT);
}

void
activate_delete_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("deleting selected tracks");

  UndoableAction * ua =
    delete_tracks_action_new (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (
    UNDO_MANAGER, ua);
}

void
change_state_dim_output (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int dim = g_variant_get_boolean (value);

  g_message ("dim is %d", dim);

  g_simple_action_set_state (action, value);
}

void
change_state_loop (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int loop = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  transport_set_loop (TRANSPORT, loop);
}

void
activate_quick_quantize (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("gvariant %p", variant);
  gsize size;
  if (variant)
    g_message ("variant: %s", g_variant_get_string (variant, &size));
  g_message ("quantize");

}

void
activate_quantize_options (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("quantize opts");

}
