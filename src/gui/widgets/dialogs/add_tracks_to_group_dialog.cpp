// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/tracklist.h"
#include "gui/widgets/dialogs/add_tracks_to_group_dialog.h"
#include "project.h"
#include "utils/gtk.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  AddTracksToGroupDialogWidget,
  add_tracks_to_group_dialog_widget,
  GTK_TYPE_DIALOG)

GroupTargetTrack *
add_tracks_to_group_dialog_widget_get_track (TracklistSelections * sel)
{
  z_return_val_if_fail (!sel->tracks_.empty (), nullptr);

  // Check if all tracks have channels and the same signal type
  auto signal_type = sel->tracks_.front ()->out_signal_type_;
  bool valid = std::all_of (
    sel->tracks_.begin (), sel->tracks_.end (), [&] (const auto &track) {
      if (!track->has_channel ())
        {
          z_info ("track {} has no channel", track->name_);
          return false;
        }
      if (track->out_signal_type_ != signal_type)
        {
          z_info ("mismatching signal type");
          return false;
        }
      return true;
    });

  if (!valid)
    return nullptr;

  auto lowest_track = sel->get_lowest_track ();
  int  lowest_pos = lowest_track->pos_;

  AddTracksToGroupDialogWidget * self = static_cast<
    AddTracksToGroupDialogWidget *> (g_object_new (
    ADD_TRACKS_TO_GROUP_DIALOG_WIDGET_TYPE, "modal", true, "title",
    _ ("Enter group name"), nullptr));

  gtk_dialog_add_button (GTK_DIALOG (self), _ ("OK"), GTK_RESPONSE_OK);
  GtkWidget * contents = gtk_dialog_get_content_area (GTK_DIALOG (self));
  GtkWidget * grid = gtk_grid_new ();
  GtkWidget * group_label = gtk_label_new (_ ("Group name"));
  GtkWidget * group_entry = gtk_entry_new ();
  auto        track_name = Track::get_unique_name (nullptr, _ ("New Group"));
  gtk_editable_set_text (GTK_EDITABLE (group_entry), track_name.c_str ());
  GtkWidget * checkbox =
    gtk_check_button_new_with_label (_ ("Move tracks under group"));
  gtk_grid_attach (GTK_GRID (grid), group_label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), group_entry, 1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), checkbox, 0, 1, 2, 1);
  gtk_box_append (GTK_BOX (contents), grid);

  int  result = z_gtk_dialog_run (GTK_DIALOG (self), false);
  bool move_inside = gtk_check_button_get_active (GTK_CHECK_BUTTON (checkbox));
  track_name = g_strdup (gtk_editable_get_text (GTK_EDITABLE (group_entry)));
  gtk_window_destroy (GTK_WINDOW (self));
  switch (result)
    {
    case GTK_RESPONSE_OK:
      try
        {
          if (signal_type == PortType::Audio)
            {
              Track::create_empty_at_idx_with_action (
                Track::Type::AudioGroup, lowest_pos + 1);
            }
          else if (signal_type == PortType::Event)
            {
              Track::create_empty_at_idx_with_action (
                Track::Type::MidiGroup, lowest_pos + 1);
            }
          else
            {
              z_info ("invalid signal type {}", ENUM_NAME (signal_type));
              return nullptr;
            }
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to create group track"));
          return nullptr;
        }
      break;
    default:
      z_info ("non-OK response");
      return NULL;
    }

  auto track = dynamic_cast<GroupTargetTrack *> (
    TRACKLIST->tracks_[lowest_pos + 1].get ());
  z_return_val_if_fail (track, nullptr);

  bool ret = track->set_name_with_action_full (track_name);
  if (ret)
    {
      int move_to_new_track_num_actions = 0;
      if (move_inside)
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<MoveTracksInsideFoldableTrackAction> (
                  *sel, track->pos_));
              move_to_new_track_num_actions++;

              /* adjust positions in previous selections */
              for (auto &cur_track : sel->tracks_)
                {
                  cur_track->pos_++;
                }
            }
          catch (const ZrythmException &e)
            {
              z_warning (e.what ());
            }
        }

      auto ua = UNDO_MANAGER->get_last_action ();
      ua->num_actions_ = 2 + move_to_new_track_num_actions;
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
