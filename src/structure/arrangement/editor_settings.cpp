// SPDX-FileCopyrightText: © 2020, 2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/editor_settings.h"

#include <json/json.h>

namespace zrythm::structure::arrangement
{
double
EditorSettings::clamp_scroll_start_x (double x)
{
  return std::max (x, 0.0);
  /*z_debug ("scrolled horizontally to {}", scroll_start_x_);*/
}

double
EditorSettings::clamp_scroll_start_y (double y)
{
  y = std::max (y, 0.0);
  {
// TODO
#if 0
    double diff = 0;
    if (this == PRJ_TIMELINE)
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
        y -= diff;
      }
#endif
  }
  /*z_debug ("scrolled vertically to {}", scroll_start_y_);*/
  return y;
}

/**
 * Appends the given deltas to the scroll x/y values.
 */
void
EditorSettings::append_scroll (double dx, double dy, bool validate)
{
  // TODO
  // scroll_start_x_ = set_scroll_start_x (scroll_start_x_ + dx);
  // scroll_start_y_ = set_scroll_start_y (scroll_start_y_ + dy);
  /*z_debug ("scrolled to ({}, {})", scroll_start_x_,
   * scroll_start_y_);*/
}

void
to_json (nlohmann::json &j, const EditorSettings &settings)
{
  j[EditorSettings::kScrollStartXKey] = settings.scroll_start_x_;
  j[EditorSettings::kScrollStartYKey] = settings.scroll_start_y_;
  j[EditorSettings::kHZoomLevelKey] = settings.hzoom_level_;
}
void
from_json (const nlohmann::json &j, EditorSettings &settings)
{
  j.at (EditorSettings::kScrollStartXKey).get_to (settings.scroll_start_x_);
  j.at (EditorSettings::kScrollStartYKey).get_to (settings.scroll_start_y_);
  j.at (EditorSettings::kHZoomLevelKey).get_to (settings.hzoom_level_);
}
}
