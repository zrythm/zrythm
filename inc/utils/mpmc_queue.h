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

/**
 * \file
 *
 * Multiple Producer Multiple Consumer lock-free
 * queue.
 */

#ifndef __UTILS_MPMC_QUEUE_H__
#define __UTILS_MPMC_QUEUE_H__

#include <stddef.h>

#include <glib.h>

/**
 * @addtogroup utils
 *
 * @{
 */

typedef struct cell_t
{
  volatile guint sequence;
  void *         data;
} cell_t;

/**
 * Multiple Producer Multiple Consumer lock-free
 * queue.
 *
 * See https://gist.github.com/x42/9aa5e737a1479bafb7f1bb96f7c64dc0
 */
typedef struct MPMCQueue
{
  cell_t * buffer;
  size_t   buffer_mask;

  volatile guint enqueue_pos;
  volatile guint dequeue_pos;

} MPMCQueue;

MPMCQueue *
mpmc_queue_new (void);

NONNULL
void
mpmc_queue_reserve (
  MPMCQueue * self,
  size_t      buffer_size);

NONNULL
void
mpmc_queue_free (MPMCQueue * self);

NONNULL
void
mpmc_queue_clear (MPMCQueue * self);

HOT NONNULL int
mpmc_queue_push_back (
  MPMCQueue *  self,
  void * const data);

HOT NONNULL int
mpmc_queue_dequeue (MPMCQueue * self, void ** data);

/**
 * @}
 */

#endif
