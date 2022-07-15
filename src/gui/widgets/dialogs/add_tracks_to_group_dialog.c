/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "gui/widgets/dialogs/add_tracks_to_group_dialog.h"
#include "project.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/midi.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  AddTracksToGroupDialogWidget,
  add_tracks_to_group_dialog_widget,
  GTK_TYPE_DIALOG)

/**
 * Creates an add_tracks_to_group dialog widget and
 * displays it.
 *
 * @return The new group track, after a create tracks
 *   action has been executed, or NULL if failure.
 */
Track *
add_tracks_to_group_dialog_widget_get_track (
  TracklistSelections * sel)
{
  /* verify that the out signal type is the same for
   * all selected tracks */
  PortType signal_type;
  g_return_val_if_fail (sel->num_tracks > 0, NULL);
  for (int i = 0; i < sel->num_tracks; i++)
    {
      Track * cur_track =
        TRACKLIST->tracks[sel->tracks[i]->pos];
      if (!track_type_has_channel (cur_track->type))
        {
          g_message (
            "track %s has no channel", cur_track->name);
          return NULL;
        }

      if (i == 0)
        signal_type = cur_track->out_signal_type;
      else if (cur_track->out_signal_type != signal_type)
        {
          g_message ("mismatching signal type");
          return NULL;
        }
    }

  Track * lowest_track =
    tracklist_selections_get_lowest_track (sel);
  int lowest_pos = lowest_track->pos;

  Track *                        track = NULL;
  AddTracksToGroupDialogWidget * self = g_object_new (
    ADD_TRACKS_TO_GROUP_DIALOG_WIDGET_TYPE, "modal", true,
    "title", _ ("Enter group name"), NULL);

  gtk_dialog_add_button (
    GTK_DIALOG (self), _ ("OK"), GTK_RESPONSE_OK);
  GtkWidget * contents =
    gtk_dialog_get_content_area (GTK_DIALOG (self));
  GtkWidget * grid = gtk_grid_new ();
  GtkWidget * group_label = gtk_label_new (_ ("Group name"));
  GtkWidget * group_entry = gtk_entry_new ();
  char *      track_name =
    track_get_unique_name (NULL, _ ("New Group"));
  gtk_editable_set_text (
    GTK_EDITABLE (group_entry), track_name);
  g_free (track_name);
  GtkWidget * checkbox = gtk_check_button_new_with_label (
    _ ("Move tracks under group"));
  gtk_grid_attach (GTK_GRID (grid), group_label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), group_entry, 1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), checkbox, 0, 1, 2, 1);
  gtk_box_append (GTK_BOX (contents), grid);

  int result = z_gtk_dialog_run (GTK_DIALOG (self), false);
  track_name = g_strdup (
    gtk_editable_get_text (GTK_EDITABLE (group_entry)));
  gtk_window_destroy (GTK_WINDOW (self));
  switch (result)
    {
    case GTK_RESPONSE_OK:
      if (signal_type == TYPE_AUDIO)
        {
          GError * err = NULL;
          bool ret = track_create_empty_at_idx_with_action (
            TRACK_TYPE_AUDIO_GROUP, lowest_track->pos + 1,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _ ("Failed to create audio group "
                   "track"));
            }
        }
      else if (signal_type == TYPE_EVENT)
        {
          GError * err = NULL;
          bool ret = track_create_empty_at_idx_with_action (
            TRACK_TYPE_MIDI_GROUP, lowest_track->pos + 1,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _ ("Failed to create MIDI group "
                   "track"));
            }
        }
      else
        {
          g_message (
            "invalid signal type %s",
            port_type_strings[signal_type].str);
          return NULL;
        }
      break;
    default:
      g_message ("non-OK response");
      return NULL;
    }

  track = TRACKLIST->tracks[lowest_pos + 1];
  g_return_val_if_fail (
    track->type == TRACK_TYPE_AUDIO_GROUP
      || track->type == TRACK_TYPE_MIDI_GROUP,
    NULL);

  bool ret =
    track_set_name_with_action_full (track, track_name);
  if (ret)
    {
      UndoableAction * ua =
        undo_manager_get_last_action (UNDO_MANAGER);
      ua->num_actions = 2;
    }

  return track;
}

static void
add_tracks_to_group_dialog_widget_class_init (
  AddTracksToGroupDialogWidgetClass * _klass)
{
}

static void
add_tracks_to_group_dialog_widget_init (
  AddTracksToGroupDialogWidget * self)
{
}
