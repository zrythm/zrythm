/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include <cmath>
#include <cstdlib>

#include "dsp/engine.h"
#include "dsp/position.h"
#include "dsp/quantize_options.h"
#include "dsp/snap_grid.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/algorithms.h"
#include "utils/objects.h"
#include "utils/pcg_rand.h"
#include "zrythm.h"

#include "gtk_wrapper.h"

/**
 * Updates snap points.
 */
void
quantize_options_update_quantize_points (QuantizeOptions * self)
{
  Position tmp, end_pos;
  position_init (&tmp);
  position_set_to_bar (&end_pos, TRANSPORT->total_bars + 1);
  self->num_q_points = 0;
  position_set_to_pos (&self->q_points[self->num_q_points++], &tmp);
  long ticks = snap_grid_get_ticks_from_length_and_type (
    self->note_length, self->note_type);
  long swing_offset =
    (long) (((float) self->swing / 100.f) * (float) ticks / 2.f);
  while (position_is_before (&tmp, &end_pos))
    {
      position_add_ticks (&tmp, ticks);

      /* delay every second point by swing */
      if ((self->num_q_points + 1) % 2 == 0)
        {
          position_add_ticks (&tmp, swing_offset);
        }

      position_set_to_pos (&self->q_points[self->num_q_points++], &tmp);
    }
}

void
quantize_options_init (QuantizeOptions * self, NoteLength note_length)
{
  self->note_length = note_length;
  self->num_q_points = 0;
  self->note_type = NoteType::NOTE_TYPE_NORMAL;
  self->amount = 100;
  self->adj_start = 1;
  self->adj_end = 0;
  self->swing = 0;
  self->rand_ticks = 0;
}

float
quantize_options_get_swing (QuantizeOptions * self)
{
  return self->swing;
}

float
quantize_options_get_amount (QuantizeOptions * self)
{
  return self->amount;
}

float
quantize_options_get_randomization (QuantizeOptions * self)
{
  return (float) self->rand_ticks;
}

void
quantize_options_set_swing (QuantizeOptions * self, float swing)
{
  self->swing = swing;
}

void
quantize_options_set_amount (QuantizeOptions * self, float amount)
{
  self->amount = amount;
}

void
quantize_options_set_randomization (QuantizeOptions * self, float randomization)
{
  self->rand_ticks = (unsigned int) round (randomization);
}

/**
 * Returns the current options as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
quantize_options_stringize (NoteLength note_length, NoteType note_type)
{
  return snap_grid_stringize_length_and_type (note_length, note_type);
}

static Position *
get_prev_point (QuantizeOptions * self, Position * pos)
{
  g_return_val_if_fail (pos->frames >= 0 && pos->ticks >= 0, NULL);

  Position * prev_point = (Position *) algorithms_binary_search_nearby (
    pos, self->q_points, (size_t) self->num_q_points, sizeof (Position),
    position_cmp_func, true, true);

  return prev_point;
}

static Position *
get_next_point (QuantizeOptions * self, Position * pos)
{
  g_return_val_if_fail (pos->frames >= 0 && pos->ticks >= 0, NULL);

  Position * next_point = (Position *) algorithms_binary_search_nearby (
    pos, self->q_points, (size_t) self->num_q_points, sizeof (Position),
    position_cmp_func, false, true);

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
double
quantize_options_quantize_position (QuantizeOptions * self, Position * pos)
{
  Position * prev_point = get_prev_point (self, pos);
  Position * next_point = get_next_point (self, pos);
  g_return_val_if_fail (prev_point && next_point, 0);

  const double upper = self->rand_ticks;
  const double lower = -self->rand_ticks;
  double       rand_double = (double) pcg_rand_u32 (gZrythm->rand);
  double       rand_ticks = fmod (rand_double, (upper - lower + 1.0)) + lower;

  /* if previous point is closer */
  double diff;
  if (pos->ticks - prev_point->ticks <= next_point->ticks - pos->ticks)
    {
      diff = prev_point->ticks - pos->ticks;
    }
  /* if next point is closer */
  else
    {
      diff = next_point->ticks - pos->ticks;
    }

  /* multiply by amount */
  diff = (diff * (double) (self->amount / 100.f));

  /* add random ticks */
  diff += rand_ticks;

  /* quantize position */
  position_add_ticks (pos, diff);

  return diff;
}

/**
 * Clones the QuantizeOptions.
 */
QuantizeOptions *
quantize_options_clone (const QuantizeOptions * src)
{
  QuantizeOptions * opts = object_new (QuantizeOptions);

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

QuantizeOptions *
quantize_options_new (void)
{
  QuantizeOptions * opts = object_new (QuantizeOptions);

  return opts;
}

/**
 * Free's the QuantizeOptions.
 */
void
quantize_options_free (QuantizeOptions * self)
{
  object_zero_and_free (self);
}
