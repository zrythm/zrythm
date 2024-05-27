// SPDX-FileCopyrightText: © 2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "gui/widgets/dialogs/add_tracks_to_group_dialog.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/midi.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  AddTracksToGroupDialogWidget,
  add_tracks_to_group_dialog_widget,
  GTK_TYPE_DIALOG)

/**
 * Creates an add_tracks_to_group dialog widget and displays it.
 *
 * @return The new group track, after a create tracks action
 *   has been executed, or NULL if failure.
 */
Track *
add_tracks_to_group_dialog_widget_get_track (TracklistSelections * sel)
{
  /* verify that the out signal type is the same for
   * all selected tracks */
  ZPortType signal_type;
  g_return_val_if_fail (sel->num_tracks > 0, NULL);
  for (int i = 0; i < sel->num_tracks; i++)
    {
      Track * cur_track = TRACKLIST->tracks[sel->tracks[i]->pos];
      if (!track_type_has_channel (cur_track->type))
        {
          g_message ("track %s has no channel", cur_track->name);
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

  Track * lowest_track = tracklist_selections_get_lowest_track (sel);
  int     lowest_pos = lowest_track->pos;

  Track *                        track = NULL;
  AddTracksToGroupDialogWidget * self = static_cast<
    AddTracksToGroupDialogWidget *> (g_object_new (
    ADD_TRACKS_TO_GROUP_DIALOG_WIDGET_TYPE, "modal", true, "title",
    _ ("Enter group name"), NULL));

  gtk_dialog_add_button (GTK_DIALOG (self), _ ("OK"), GTK_RESPONSE_OK);
  GtkWidget * contents = gtk_dialog_get_content_area (GTK_DIALOG (self));
  GtkWidget * grid = gtk_grid_new ();
  GtkWidget * group_label = gtk_label_new (_ ("Group name"));
  GtkWidget * group_entry = gtk_entry_new ();
  char *      track_name = track_get_unique_name (NULL, _ ("New Group"));
  gtk_editable_set_text (GTK_EDITABLE (group_entry), track_name);
  g_free (track_name);
  GtkWidget * checkbox =
    gtk_check_button_new_with_label (_ ("Move tracks under group"));
  gtk_grid_attach (GTK_GRID (grid), group_label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), group_entry, 1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), checkbox, 0, 1, 2, 1);
  gtk_box_append (GTK_BOX (contents), grid);

  int  result = z_gtk_dialog_run (GTK_DIALOG (self), false);
  bool move_inside = gtk_check_button_get_active (GTK_CHECK_BUTTON (checkbox));
  track_name = g_strdup (gtk_editable_get_text (GTK_EDITABLE (group_entry)));
  g_return_val_if_fail (track_name, NULL);
  gtk_window_destroy (GTK_WINDOW (self));
  switch (result)
    {
    case GTK_RESPONSE_OK:
      if (signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          GError * err = NULL;
          bool     ret = track_create_empty_at_idx_with_action (
            TrackType::TRACK_TYPE_AUDIO_GROUP, lowest_track->pos + 1, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _ ("Failed to create audio group "
                   "track"));
            }
        }
      else if (signal_type == ZPortType::Z_PORT_TYPE_EVENT)
        {
          GError * err = NULL;
          bool     ret = track_create_empty_at_idx_with_action (
            TrackType::TRACK_TYPE_MIDI_GROUP, lowest_track->pos + 1, &err);
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
          g_message ("invalid signal type %s", ENUM_NAME (signal_type));
          return NULL;
        }
      break;
    default:
      g_message ("non-OK response");
      return NULL;
    }

  track = TRACKLIST->tracks[lowest_pos + 1];
  g_return_val_if_fail (
    track->type == TrackType::TRACK_TYPE_AUDIO_GROUP
      || track->type == TrackType::TRACK_TYPE_MIDI_GROUP,
    NULL);

  bool ret = track_set_name_with_action_full (track, track_name);
  if (ret)
    {
      int move_to_new_track_num_actions = 0;
      if (move_inside)
        {
          GError * err = NULL;
          bool     success = tracklist_selections_action_perform_move_inside (
            sel, PORT_CONNECTIONS_MGR, track->pos, &err);
          if (success)
            {
              move_to_new_track_num_actions++;

              /* adjust positions in previous selections */
              for (int i = 0; i < sel->num_tracks; i++)
                {
                  Track * cur_track = sel->tracks[i];
                  cur_track->pos++;
                }
            }
        }

      UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
      ua->num_actions = 2 + move_to_new_track_num_actions;
    }

  return track;
}

static void
add_tracks_to_group_dialog_widget_class_init (
  AddTracksToGroupDialogWidgetClass * _klass)
{
}

static void
add_tracks_to_group_dialog_widget_init (AddTracksToGroupDialogWidget * self)
{
}
