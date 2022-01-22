/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/snap_grid.h"
#include "audio/transport.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/algorithms.h"
#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

int
snap_grid_get_ticks_from_length_and_type (
  NoteLength length,
  NoteType   type)
{
  int ticks = 0;
  switch (length)
    {
    case NOTE_LENGTH_BAR:
      g_return_val_if_fail (
        TRANSPORT && TRANSPORT->ticks_per_bar > 0,
        -1);
      ticks = TRANSPORT->ticks_per_bar;
      break;
    case NOTE_LENGTH_BEAT:
      g_return_val_if_fail (
        TRANSPORT && TRANSPORT->ticks_per_beat > 0,
        -1);
      ticks = TRANSPORT->ticks_per_beat;
      break;
    case NOTE_LENGTH_2_1:
      ticks = 8 * TICKS_PER_QUARTER_NOTE;
      break;
    case NOTE_LENGTH_1_1:
      ticks = 4 * TICKS_PER_QUARTER_NOTE;
      break;
    case NOTE_LENGTH_1_2:
      ticks = 2 * TICKS_PER_QUARTER_NOTE;
      break;
    case NOTE_LENGTH_1_4:
      ticks = TICKS_PER_QUARTER_NOTE;
      break;
    case NOTE_LENGTH_1_8:
      ticks = TICKS_PER_QUARTER_NOTE / 2;
      break;
    case NOTE_LENGTH_1_16:
      ticks = TICKS_PER_QUARTER_NOTE / 4;
      break;
    case NOTE_LENGTH_1_32:
      ticks = TICKS_PER_QUARTER_NOTE / 8;
      break;
    case NOTE_LENGTH_1_64:
      ticks = TICKS_PER_QUARTER_NOTE / 16;
      break;
    case NOTE_LENGTH_1_128:
      ticks = TICKS_PER_QUARTER_NOTE / 32;
      break;
    default:
      g_return_val_if_reached (-1);
    }

  switch (type)
    {
    case NOTE_TYPE_NORMAL:
      break;
    case NOTE_TYPE_DOTTED:
      ticks = 3 * ticks;
      g_return_val_if_fail (
        ticks % 2 == 0, -1);
      ticks = ticks / 2;
      break;
    case NOTE_TYPE_TRIPLET:
      ticks = 2 * ticks;
      g_return_val_if_fail (
        ticks % 3 == 0, -1);
      ticks = ticks / 3;
      break;
    }

  return ticks;
}

/**
 * Gets a snap point's length in ticks.
 */
int
snap_grid_get_snap_ticks (
  const SnapGrid * self)
{
  if (self->snap_adaptive)
    {
      if (!ZRYTHM_HAVE_UI || ZRYTHM_TESTING)
        {
          g_critical ("can only be used with UI");
          return -1;
        }

      RulerWidget * ruler = NULL;
      if (self->type == SNAP_GRID_TYPE_TIMELINE)
        {
          ruler = MW_RULER;
        }
      else
        {
          ruler = EDITOR_RULER;
        }

      /* get intervals used in drawing */
      int sixteenth_interval =
        ruler_widget_get_sixteenth_interval (
          ruler);
      int beat_interval =
        ruler_widget_get_beat_interval (
          ruler);
      int bar_interval =
        (int)
        MAX ((RW_PX_TO_HIDE_BEATS) /
             (double) ruler->px_per_bar, 1.0);

      /* attempt to snap at smallest interval */
      if (sixteenth_interval > 0)
        {
          return
            sixteenth_interval *
            snap_grid_get_ticks_from_length_and_type (
              NOTE_LENGTH_1_16, self->snap_note_type);
        }
      else if (beat_interval > 0)
        {
          return
            beat_interval *
            snap_grid_get_ticks_from_length_and_type (
              NOTE_LENGTH_BEAT, self->snap_note_type);
        }
      else
        {
          return
            bar_interval *
            snap_grid_get_ticks_from_length_and_type (
              NOTE_LENGTH_BAR, self->snap_note_type);
        }
    }
  else
    {
      return
        snap_grid_get_ticks_from_length_and_type (
          self->snap_note_length,
          self->snap_note_type);
    }
}

double
snap_grid_get_snap_frames (
  const SnapGrid * self)
{
  int snap_ticks = snap_grid_get_snap_ticks (self);
  return
    AUDIO_ENGINE->frames_per_tick
    * (double) snap_ticks;
}

/**
 * Gets a the default length in ticks.
 */
int
snap_grid_get_default_ticks (
  SnapGrid * self)
{
  if (self->length_type == NOTE_LENGTH_LINK)
    {
      return snap_grid_get_snap_ticks (self);
    }
  else if (self->length_type ==
             NOTE_LENGTH_LAST_OBJECT)
    {
      double last_obj_length = 0.0;
      if (self->type == SNAP_GRID_TYPE_TIMELINE)
        {
          last_obj_length =
            g_settings_get_double (
              S_UI, "timeline-last-object-length");
        }
      else if (self->type == SNAP_GRID_TYPE_EDITOR)
        {
          last_obj_length =
            g_settings_get_double (
              S_UI, "editor-last-object-length");
        }
      return (int) last_obj_length;
    }
  else
    {
      return
        snap_grid_get_ticks_from_length_and_type (
          self->default_note_length,
          self->default_note_type);
    }
}

void
snap_grid_init (
  SnapGrid *   self,
  SnapGridType type,
  NoteLength   note_length)
{
  self->schema_version = SNAP_GRID_SCHEMA_VERSION;
  self->type = type;
  self->snap_note_length = note_length;
  self->snap_note_type = NOTE_TYPE_NORMAL;
  self->default_note_length = note_length;
  self->default_note_type = NOTE_TYPE_NORMAL;
  self->snap_to_grid = true;
  self->length_type = NOTE_LENGTH_LINK;
}

static const char *
get_note_type_str (
  NoteType type)
{
  return note_type_short_strings[type].str;
}

static const char *
get_note_length_str (
  NoteLength length)
{
  return note_length_strings[length].str;
}

/**
 * Returns the grid intensity as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize_length_and_type (
  NoteLength note_length,
  NoteType   note_type)
{
  const char * c =
    get_note_type_str (note_type);
  const char * first_part =
    get_note_length_str (note_length);

  return g_strdup_printf ("%s%s", first_part, c);
}

/**
 * Returns the grid intensity as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (
  SnapGrid * self)
{
  if (self->snap_adaptive)
    {
      return g_strdup (_("Adaptive"));
    }
  else
    {
      return
        snap_grid_stringize_length_and_type (
          self->snap_note_length,
          self->snap_note_type);
    }
}

/**
 * Returns the next or previous SnapGrid Point.
 *
 * @param self Snap grid to search in.
 * @param pos Position to search for.
 * @param return_prev 1 to return the previous
 * element or 0 to return the next.
 */
bool
snap_grid_get_nearby_snap_point (
  Position *             ret_pos,
  const SnapGrid * const self,
  const Position *       pos,
  const bool             return_prev)
{
  g_return_val_if_fail (
    pos->frames >= 0 && pos->ticks >= 0, false);

  position_set_to_pos (ret_pos, pos);
  double snap_ticks =
    snap_grid_get_snap_ticks (self);
  double ticks_from_prev =
    fmod (pos->ticks, snap_ticks);
  if (return_prev)
    {
      position_add_ticks (
        ret_pos, - ticks_from_prev);
    }
  else
    {
      double ticks_to_next =
        snap_ticks - ticks_from_prev;
      position_add_ticks (
        ret_pos, ticks_to_next);
    }

  return true;
}

SnapGrid *
snap_grid_clone (
  SnapGrid * src)
{
  SnapGrid * self = object_new (SnapGrid);
  self->schema_version = SNAP_GRID_SCHEMA_VERSION;

  self->type = src->type;
  self->snap_note_length = src->snap_note_length;
  self->snap_note_type = src->snap_note_type;
  self->snap_adaptive = src->snap_adaptive;
  self->default_note_length =
    src->default_note_length;
  self->default_note_type =
    src->default_note_type;
  self->default_adaptive = src->default_adaptive;
  self->length_type = src->length_type;
  self->snap_to_grid = src->snap_to_grid;
  self->snap_to_grid_keep_offset =
    src->snap_to_grid_keep_offset;
  self->snap_to_events = src->snap_to_events;

  return self;
}

SnapGrid *
snap_grid_new (void)
{
  SnapGrid * self = object_new (SnapGrid);
  self->schema_version = SNAP_GRID_SCHEMA_VERSION;

  return self;
}

void
snap_grid_free (
  SnapGrid * self)
{
  object_zero_and_free (self);
}
