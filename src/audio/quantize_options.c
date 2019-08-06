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

#include <stdlib.h>

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
  long swing_offset =
    (self->swing / 100.f) *
    ticks / 2.f;
  while (position_is_before (&tmp, &end_pos))
    {
      position_add_ticks (
        &tmp, ticks);

      /* delay every second point by swing */
      if ((self->num_q_points + 1) % 2 == 0)
        {
          position_add_ticks (
            &tmp, swing_offset);
        }

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

float
quantize_options_get_swing (
  QuantizeOptions * self)
{
  return self->swing;
}

float
quantize_options_get_amount (
  QuantizeOptions * self)
{
  return self->amount;
}

float
quantize_options_get_randomization (
  QuantizeOptions * self)
{
  return self->rand_ticks;
}

void
quantize_options_set_swing (
  QuantizeOptions * self,
  float             swing)
{
  self->swing = swing;
}

void
quantize_options_set_amount (
  QuantizeOptions * self,
  float             amount)
{
  self->amount = amount;
}

void
quantize_options_set_randomization (
  QuantizeOptions * self,
  float             randomization)
{
  self->rand_ticks = randomization;
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
 * Clones the QuantizeOptions.
 */
QuantizeOptions *
quantize_options_clone (
  const QuantizeOptions * src)
{
  QuantizeOptions * opts =
    calloc (1, sizeof (QuantizeOptions));

  opts->note_length = src->note_length;
  opts->note_type = src->note_type;
  opts->amount = src->amount;
  opts->adj_start = src->adj_start;
  opts->adj_end = src->adj_end;
  opts->swing = src->swing;
  opts->rand_ticks = src->rand_ticks;

  quantize_options_update_quantize_points (opts);

  return opts;
}

static Position *
get_prev_point (
  QuantizeOptions * self,
  Position *        pos)
{
  Position * prev_point = NULL;
  algorithms_binary_search_nearby (
    self->q_points, pos, 1, 1,
    self->num_q_points, Position *,
    position_compare, &, prev_point, NULL);

  return prev_point;
}

static Position *
get_next_point (
  QuantizeOptions * self,
  Position *        pos)
{
  Position * next_point = NULL;
  algorithms_binary_search_nearby (
    self->q_points, pos, 0, 1,
    self->num_q_points, Position *,
    position_compare, &, next_point, NULL);

  return next_point;
}

/**
 * Quantizes the given Position using the given
 * QuantizeOptions.
 *
 * This assumes that the start/end check has been
 * done already and it ignores the adjust_start and
 * adjust_end options.
 *
 * @return The amount of ticks moved (negative for
 *   backwards).
 */
int
quantize_options_quantize_position (
  QuantizeOptions * self,
  Position *        pos)
{
  Position * prev_point =
    get_prev_point (self, pos);
  Position * next_point =
    get_next_point (self, pos);

  const int upper = self->rand_ticks;
  const int lower = - self->rand_ticks;
  int rand_ticks =
    (random() %
    (upper - lower + 1)) + lower;

  /* if previous point is closer */
  int diff;
  if (pos->total_ticks -
        prev_point->total_ticks <=
      next_point->total_ticks -
        pos->total_ticks)
    {
      diff =
        prev_point->total_ticks -
        pos->total_ticks;
    }
  /* if next point is closer */
  else
    {
      diff =
        next_point->total_ticks -
        pos->total_ticks;
    }

  /* multiply by amount */
  diff *= (self->amount / 100.f);

  /* add random ticks */
  diff += rand_ticks;

  /* quantize position */
  position_add_ticks (
    pos, diff);

  return diff;
}

/**
 * Free's the QuantizeOptions.
 */
void
quantize_options_free (
  QuantizeOptions * self)
{
  free (self);
}
