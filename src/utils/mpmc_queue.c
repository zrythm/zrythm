/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdlib.h>

#include "utils/mpmc_queue.h"
#include "utils/objects.h"

CONST
static size_t
power_of_two_size (size_t sz)
{
  int32_t power_of_two;
  for (power_of_two = 1; 1U << power_of_two < sz;
       ++power_of_two)
    ;
  return 1U << power_of_two;
}

void
mpmc_queue_reserve (
  MPMCQueue * self,
  size_t      buffer_size)
{
  buffer_size = power_of_two_size (buffer_size);
  g_return_if_fail (
    (buffer_size >= 2)
    && ((buffer_size & (buffer_size - 1)) == 0));

  if (self->buffer_mask >= buffer_size - 1)
    return;

  if (self->buffer)
    free (self->buffer);

  self->buffer = object_new_n (buffer_size, cell_t);
  self->buffer_mask = buffer_size - 1;

  mpmc_queue_clear (self);
}

MPMCQueue *
mpmc_queue_new (void)
{
  MPMCQueue * self = object_new (MPMCQueue);

  mpmc_queue_reserve (self, 8);

  return self;
}

void
mpmc_queue_free (MPMCQueue * self)
{
  free (self->buffer);

  free (self);
}

void
mpmc_queue_clear (MPMCQueue * self)
{
  for (size_t i = 0; i <= self->buffer_mask; ++i)
    {
      g_atomic_int_set (
        &self->buffer[i].sequence, (guint) i);
    }
  g_atomic_int_set (&self->enqueue_pos, 0);
  g_atomic_int_set (&self->dequeue_pos, 0);
}

int
mpmc_queue_push_back (
  MPMCQueue *  self,
  void * const data)
{
  cell_t * cell;
  gint pos = g_atomic_int_get (&self->enqueue_pos);
  for (;;)
    {
      cell =
        &self->buffer
           [(size_t) pos & self->buffer_mask];
      guint seq =
        (guint) g_atomic_int_get (&cell->sequence);
      intptr_t dif = (intptr_t) seq - (intptr_t) pos;
      if (dif == 0)
        {
          if (g_atomic_int_compare_and_exchange (
                &self->enqueue_pos, pos, (pos + 1)))
            {
              break;
            }
        }
      else if (G_UNLIKELY (dif < 0))
        {
          g_return_val_if_reached (0);
        }
      else
        {
          pos =
            g_atomic_int_get (&self->enqueue_pos);
        }
    }
  cell->data = data;
  g_atomic_int_set (&cell->sequence, pos + 1);

  return 1;
}

int
mpmc_queue_dequeue (MPMCQueue * self, void ** data)
{
  cell_t * cell;
  gint pos = g_atomic_int_get (&self->dequeue_pos);
  for (;;)
    {
      cell =
        &self->buffer
           [(size_t) pos & self->buffer_mask];
      guint seq =
        (guint) g_atomic_int_get (&cell->sequence);
      intptr_t dif =
        (intptr_t) seq - (intptr_t) (pos + 1);
      if (dif == 0)
        {
          if (g_atomic_int_compare_and_exchange (
                &self->dequeue_pos, pos, (pos + 1)))
            break;
        }
      else if (dif < 0)
        {
          return 0;
        }
      else
        {
          pos =
            g_atomic_int_get (&self->dequeue_pos);
        }
    }
  *data = cell->data;
  g_atomic_int_set (
    &cell->sequence,
    pos + (gint) self->buffer_mask + 1);

  return 1;
}
