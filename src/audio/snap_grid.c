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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/snap_grid.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/algorithms.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

/**
 * Gets given note length and type in ticks.
 */
int
snap_grid_get_note_ticks (
  NoteLength note_length,
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
  g_warn_if_reached ();
  return -1;
}

static void
add_snap_point (
  SnapGrid * self,
  Position * snap_point)
{
  array_double_size_if_full (
    self->snap_points, self->num_snap_points,
    self->snap_points_size, Position);
  position_set_to_pos (
    &self->snap_points[self->num_snap_points++],
    snap_point);
}

/**
 * Updates snap points.
 */
void
snap_grid_update_snap_points (SnapGrid * self)
{
  POSITION_INIT_ON_STACK (tmp);
  Position end_pos;
  position_set_to_bar (
    &end_pos,
    TRANSPORT->total_bars + 1);
  self->num_snap_points = 0;
  add_snap_point (self, &tmp);
  long ticks =
    snap_grid_get_note_ticks (
      self->note_length, self->note_type);
  g_warn_if_fail (TRANSPORT->ticks_per_bar > 0);
  while (position_is_before (&tmp, &end_pos))
    {
      position_add_ticks (&tmp, ticks);
      add_snap_point (self, &tmp);
    }
}

void
snap_grid_init (
  SnapGrid *   self,
  NoteLength   note_length)
{
  self->grid_auto = 1;
  self->note_length = note_length;
  self->num_snap_points = 0;
  self->note_type = NOTE_TYPE_NORMAL;
  self->snap_to_grid = 1;

  self->snap_points =
    calloc (1, sizeof (Position));
  self->snap_points_size = 1;
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
 * Returns the grid intensity as a human-readable string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (NoteLength note_length,
                     NoteType   note_type)
{
  const char * c =
    get_note_type_str (note_type);
  const char * first_part =
    get_note_length_str (note_length);

  return g_strdup_printf ("%s%s",
                          first_part,
                          c);
}

/**
 * Returns the next or previous SnapGrid Point.
 *
 * @param self Snap grid to search in.
 * @param pos Position to search for.
 * @param return_prev 1 to return the previous
 * element or 0 to return the next.
 */
Position *
snap_grid_get_nearby_snap_point (
  SnapGrid * self,
  const Position * pos,
  const int        return_prev)
{
  Position * ret_pos = NULL;
  algorithms_binary_search_nearby (
    self->snap_points, pos, return_prev, 0,
    self->num_snap_points, Position *,
    position_compare, &, ret_pos, NULL);

  return ret_pos;

  // init values
  /*int first = 0;*/
  /*int last = self->num_snap_points;*/
  /*int middle = (first + last) / 2;*/
  /*int pivot_is_before_pos, pivot_succ_is_before_pos;*/
  /*Position *pivot, *pivot_succ;*/

  /*// return if SnapGrid has no entries*/
  /*if (first == last)*/
  /*{*/
    /*return NULL;*/
  /*}*/

  /*// search loops, exit if pos is not in array*/
  /*while (first <= last)*/
  /*{*/
    /*pivot = &self->snap_points[middle];*/
    /*pivot_succ = NULL;*/
    /*pivot_succ_is_before_pos = 0;*/

    /*if (middle == 0 && return_prev) {*/
      /*return &self->snap_points[0];*/
    /*}*/

    /*// Return next/previous item if pivot is the searched position*/
    /*if (position_compare(pivot, pos) == 0) {*/
      /*if (return_prev)*/
        /*return &self->snap_points[middle - 1];*/
      /*else*/
        /*return &self->snap_points[middle + 1];*/
    /*}*/

    /*// Select pivot successor if possible*/
    /*if (middle < last)*/
    /*{*/
      /*pivot_succ = &self->snap_points[middle + 1];*/
      /*pivot_succ_is_before_pos =*/
          /*position_compare(*/
              /*pivot_succ, pos) <= 0;*/
    /*}*/
    /*pivot_is_before_pos =*/
        /*position_compare(*/
            /*pivot, pos) <= 0;*/

    /*// if pivot and pivot_succ are before pos, search in the second half on next iteration*/
    /*if (pivot_is_before_pos && pivot_succ_is_before_pos)*/
    /*{*/
      /*first = middle + 1;*/
    /*}*/
    /*else if (pivot_is_before_pos) // pos is between pivot and pivot_succ*/
    /*{*/
      /*if (return_prev) {*/
        /*return pivot;*/
      /*} else {*/
        /*return pivot_succ;*/
      /*}*/
    /*}*/
    /*else { // if pivot_succ and pivot are behind pos, search in the first half on next iteration*/
      /*last = middle;*/
    /*}*/

    /*// recalculate middle position*/
    /*middle = (first + last) / 2;*/
  /*}*/

  /*return NULL;*/
}
