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
#include "audio/quantize_options.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/algorithms.h"

#include <gtk/gtk.h>

/**
 * Updates snap points.
 */
void
quantize_options_update_quantize_points (
  QuantizeOptions * self)
{
  Position tmp, end_pos;
  position_init (&tmp);
  position_set_to_bar (
    &end_pos,
    TRANSPORT->total_bars + 1);
  self->num_q_points = 0;
  position_set_to_pos (
    &self->q_points[self->num_q_points++],
    &tmp);
  long ticks =
    snap_grid_get_note_ticks (
      self->note_length,
      self->note_type);
  while (position_compare (&tmp, &end_pos)
           < 0)
    {
      position_add_ticks (
        &tmp,
        ticks);
      /*position_print (&tmp);*/
      position_set_to_pos (
        &self->q_points[self->num_q_points++],
        &tmp);
    }
}

void
quantize_options_init (QuantizeOptions *   self,
                NoteLength   note_length)
{
  self->note_length = note_length;
  self->num_q_points = 0;
  self->note_type = NOTE_TYPE_NORMAL;
  self->amount = 100;
  self->adj_start = 1;
  self->adj_end = 0;
  self->swing = 0;
  self->rand_ticks = 0;
}

/**
 * Returns the current options as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
quantize_options_stringize (NoteLength note_length,
                     NoteType   note_type)
{
  char * c = NULL;
  char * first_part = NULL;
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

/**
 * Returns the next or previous QuantizeOptions Point.
 *
 * @param self Snap grid to search in.
 * @param pos Position to search for.
 * @param return_prev 1 to return the previous
 * element or 0 to return the next.
 */
Position *
quantize_options_get_nearby_quantize_point (
  QuantizeOptions * self,
  const Position * pos,
  const int        return_prev)
{
  Position * ret_pos = NULL;
  algorithms_binary_search_nearby (
    self->q_points, pos, return_prev,
    self->num_q_points, Position *,
    position_compare, &, ret_pos, NULL);

  return ret_pos;
}
