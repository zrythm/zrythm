/*
 * project/snap_grid.c - Snap Grid info
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/snap_grid.h"
#include "audio/transport.h"
#include "project.h"

#include <gtk/gtk.h>

/**
 * Gets given note length and type in ticks.
 */
int
snap_grid_get_note_ticks (NoteLength note_length,
                          NoteType   note_type)
{
  switch (note_length)
    {
    case NOTE_LENGTH_2_1:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return 8 * TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_DOTTED:
          return 12 * TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_TRIPLET:
          return (16 * TICKS_PER_QUARTER_NOTE) / 3;
          break;
        }
      break;
    case NOTE_LENGTH_1_1:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return 4 * TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_DOTTED:
          return 6 * TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_TRIPLET:
          return (8 * TICKS_PER_QUARTER_NOTE) / 3;
          break;
        }
      break;
    case NOTE_LENGTH_1_2:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return 2 * TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_DOTTED:
          return 3 * TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_TRIPLET:
          return (4 * TICKS_PER_QUARTER_NOTE) / 3;
          break;
        }
      break;
    case NOTE_LENGTH_1_4:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return TICKS_PER_QUARTER_NOTE;
          break;
        case NOTE_TYPE_DOTTED:
          return (3 * TICKS_PER_QUARTER_NOTE) / 2;
          break;
        case NOTE_TYPE_TRIPLET:
          return (2 * TICKS_PER_QUARTER_NOTE) / 3;
          break;
        }
      break;
    case NOTE_LENGTH_1_8:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return TICKS_PER_QUARTER_NOTE / 2;
          break;
        case NOTE_TYPE_DOTTED:
          return (3 * TICKS_PER_QUARTER_NOTE) / 4;
          break;
        case NOTE_TYPE_TRIPLET:
          return TICKS_PER_QUARTER_NOTE / 3;
          break;
        }
      break;
    case NOTE_LENGTH_1_16:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return TICKS_PER_QUARTER_NOTE / 4;
          break;
        case NOTE_TYPE_DOTTED:
          return (3 * TICKS_PER_QUARTER_NOTE) / 8;
          break;
        case NOTE_TYPE_TRIPLET:
          return TICKS_PER_QUARTER_NOTE / 6;
          break;
        }
      break;
    case NOTE_LENGTH_1_32:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return TICKS_PER_QUARTER_NOTE / 8;
          break;
        case NOTE_TYPE_DOTTED:
          return (3 * TICKS_PER_QUARTER_NOTE) / 16;
          break;
        case NOTE_TYPE_TRIPLET:
          return TICKS_PER_QUARTER_NOTE / 12;
          break;
        }
      break;
    case NOTE_LENGTH_1_64:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return TICKS_PER_QUARTER_NOTE / 16;
          break;
        case NOTE_TYPE_DOTTED:
          return (3 * TICKS_PER_QUARTER_NOTE) / 32;
          break;
        case NOTE_TYPE_TRIPLET:
          return TICKS_PER_QUARTER_NOTE / 24;
          break;
        }
      break;
    case NOTE_LENGTH_1_128:
      switch (note_type)
        {
        case NOTE_TYPE_NORMAL:
          return TICKS_PER_QUARTER_NOTE / 32;
          break;
        case NOTE_TYPE_DOTTED:
          return (3 * TICKS_PER_QUARTER_NOTE) / 64;
          break;
        case NOTE_TYPE_TRIPLET:
          return TICKS_PER_QUARTER_NOTE / 48;
          break;
        }
      break;
    }
  g_assert_not_reached ();
}

/**
 * Updates snap points.
 */
void
snap_grid_update_snap_points (SnapGrid * self)
{
  Position tmp, end_pos;
  position_init (&tmp);
  position_set_to_bar (&end_pos,
                       TRANSPORT->total_bars + 1);
  self->num_snap_points = 0;
  position_set_to_pos (
    &self->snap_points[self->num_snap_points++],
    &tmp);
  while (position_compare (&tmp, &end_pos)
           < 0)
    {
      position_add_ticks (
        &tmp,
        snap_grid_get_note_ticks (self->note_length,
                                  self->note_type));
      /*position_print (&tmp);*/
      position_set_to_pos (
        &self->snap_points[self->num_snap_points++],
        &tmp);
    }
}

void
snap_grid_init (SnapGrid *   self,
                NoteLength   note_length)
{
  self->grid_auto = 1;
  self->note_length = note_length;
  self->num_snap_points = 0;
  self->note_type = NOTE_TYPE_NORMAL;
  self->snap_to_grid = 1;
}

/**
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (NoteLength note_length,
                     NoteType   note_type)
{
  char * c;
  char * first_part;
  switch (note_type)
    {
      case NOTE_TYPE_NORMAL:
        c = "";
        break;
      case NOTE_TYPE_DOTTED:
        c = ".";
        break;
      case NOTE_TYPE_TRIPLET:
        c = "t";
        break;
    }
  switch (note_length)
    {
    case NOTE_LENGTH_2_1:
      first_part = "2/1";
      break;
    case NOTE_LENGTH_1_1:
      first_part = "1/1";
      break;
    case NOTE_LENGTH_1_2:
      first_part = "1/2";
      break;
    case NOTE_LENGTH_1_4:
      first_part = "1/4";
      break;
    case NOTE_LENGTH_1_8:
      first_part = "1/8";
      break;
    case NOTE_LENGTH_1_16:
      first_part = "1/16";
      break;
    case NOTE_LENGTH_1_32:
      first_part = "1/32";
      break;
    case NOTE_LENGTH_1_64:
      first_part = "1/64";
      break;
    case NOTE_LENGTH_1_128:
      first_part = "1/128";
      break;
    }

  return g_strdup_printf ("%s%s",
                          first_part,
                          c);
}
