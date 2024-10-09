// SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/editor_settings.h"
#include "gui/backend/backend/project.h"

void
EditorSettings::set_scroll_start_x (int x, bool validate)
{
  scroll_start_x_ = MAX (x, 0);
  if (validate)
    {
      if (this == PRJ_TIMELINE.get ())
        {
        }
    }
  /*z_debug ("scrolled horizontally to {}", scroll_start_x_);*/
}

void
EditorSettings::set_scroll_start_y (int y, bool validate)
{
#if 0
  scroll_start_y_ = MAX (y, 0);
  if (validate)
    {
      int diff = 0;
      if (this == PRJ_TIMELINE.get ())
        {
          int tracklist_height =
            gtk_widget_get_height (GTK_WIDGET (MW_TRACKLIST->unpinned_box));
          int tracklist_scroll_height =
            gtk_widget_get_height (GTK_WIDGET (MW_TRACKLIST->unpinned_scroll));
          diff = (scroll_start_y_ + tracklist_scroll_height) - tracklist_height;
        }
      else if (this == PIANO_ROLL)
        {
          int piano_roll_keys_height = gtk_widget_get_height (
            GTK_WIDGET (MW_MIDI_EDITOR_SPACE->piano_roll_keys));
          int piano_roll_keys_scroll_height = gtk_widget_get_height (
            GTK_WIDGET (MW_MIDI_EDITOR_SPACE->piano_roll_keys_scroll));
          diff =
            (scroll_start_y_ + piano_roll_keys_scroll_height)
            - piano_roll_keys_height;
        }
      else if (this == CHORD_EDITOR)
        {
          int chord_keys_height = gtk_widget_get_height (
            GTK_WIDGET (MW_CHORD_EDITOR_SPACE->chord_keys_box));
          int chord_keys_scroll_height = gtk_widget_get_height (
            GTK_WIDGET (MW_CHORD_EDITOR_SPACE->chord_keys_scroll));
          diff =
            (scroll_start_y_ + chord_keys_scroll_height) - chord_keys_height;
        }

      if (diff > 0)
        {
          scroll_start_y_ -= diff;
        }
    }
  /*z_debug ("scrolled vertically to {}", scroll_start_y_);*/
#endif
}

/**
 * Appends the given deltas to the scroll x/y values.
 */
void
EditorSettings::append_scroll (int dx, int dy, bool validate)
{
  set_scroll_start_x (scroll_start_x_ + dx, validate);
  set_scroll_start_y (scroll_start_y_ + dy, validate);
  /*z_debug ("scrolled to ({}, {})", scroll_start_x_,
   * scroll_start_y_);*/
}
