// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/editor_settings.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/debug.h"
#include "zrythm_app.h"

void
editor_settings_init (EditorSettings * self)
{
  self->schema_version = EDITOR_SETTINGS_SCHEMA_VERSION;
  self->hzoom_level = 1.0;
}

void
editor_settings_set_scroll_start_x (
  EditorSettings * self,
  int              x,
  bool             validate)
{
  self->scroll_start_x = MAX (x, 0);
  if (validate)
    {
      if (self == &PRJ_TIMELINE->editor_settings)
        {
        }
    }
  g_debug (
    "scrolled horizontally to %d", self->scroll_start_x);
}

void
editor_settings_set_scroll_start_y (
  EditorSettings * self,
  int              y,
  bool             validate)
{
  self->scroll_start_y = MAX (y, 0);
  if (validate)
    {
      int diff = 0;
      if (self == &PRJ_TIMELINE->editor_settings)
        {
          int tracklist_height = gtk_widget_get_height (
            GTK_WIDGET (MW_TRACKLIST->unpinned_box));
          int tracklist_scroll_height = gtk_widget_get_height (
            GTK_WIDGET (MW_TRACKLIST->unpinned_scroll));
          diff =
            (self->scroll_start_y + tracklist_scroll_height)
            - tracklist_height;
        }
      else if (self == &PIANO_ROLL->editor_settings)
        {
          int piano_roll_keys_height = gtk_widget_get_height (
            GTK_WIDGET (MW_MIDI_EDITOR_SPACE->piano_roll_keys));
          int piano_roll_keys_scroll_height =
            gtk_widget_get_height (GTK_WIDGET (
              MW_MIDI_EDITOR_SPACE->piano_roll_keys_scroll));
          diff =
            (self->scroll_start_y
             + piano_roll_keys_scroll_height)
            - piano_roll_keys_height;
        }
      else if (self == &CHORD_EDITOR->editor_settings)
        {
          int chord_keys_height = gtk_widget_get_height (
            GTK_WIDGET (MW_CHORD_EDITOR_SPACE->chord_keys_box));
          int chord_keys_scroll_height =
            gtk_widget_get_height (GTK_WIDGET (
              MW_CHORD_EDITOR_SPACE->chord_keys_scroll));
          diff =
            (self->scroll_start_y + chord_keys_scroll_height)
            - chord_keys_height;
        }

      if (diff > 0)
        {
          self->scroll_start_y -= diff;
        }
    }
  g_debug ("scrolled vertically to %d", self->scroll_start_y);
}

/**
 * Appends the given deltas to the scroll x/y values.
 */
void
editor_settings_append_scroll (
  EditorSettings * self,
  int              dx,
  int              dy,
  bool             validate)
{
  editor_settings_set_scroll_start_x (
    self, self->scroll_start_x + dx, validate);
  editor_settings_set_scroll_start_y (
    self, self->scroll_start_y + dy, validate);
  g_debug (
    "scrolled to (%d, %d)", self->scroll_start_x,
    self->scroll_start_y);
}
