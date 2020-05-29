/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex@zrythm.org>
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

#ifndef __UTILS_LOG_H__
#define __UTILS_LOG_H__

#include <stdbool.h>

#include <gtk/gtk.h>

typedef struct _LogViewerWidget LogViewerWidget;
typedef struct MPMCQueue MPMCQueue;
typedef struct ObjectPool ObjectPool;

/**
 * @addtogroup utils
 *
 * @{
 */

#define LOG (&ZRYTHM->log)

typedef struct Log
{
  FILE * logfile;

  /* Buffers to fill in */
  GtkTextBuffer * messages_buf;
  GtkTextBuffer * warnings_buf;
  GtkTextBuffer * critical_buf;

  /** Message queue, for when messages are sent
   * from a non-gtk thread. */
  MPMCQueue *     mqueue;

  /** Object pool for the queue. */
  ObjectPool *    obj_pool;

  bool            initialized;

  /** Currently opened log viewer. */
  LogViewerWidget * viewer;
} Log;

/**
 * Initializes logging to a file.
 *
 * This must be called from the GTK thread.
 */
void
log_init_writer_idle (
  Log * self);

/**
 * Idle callback.
 */
int
log_idle_cb (
  Log * self);

/**
 * Returns the last \ref n lines as a newly
 * allocated string.
 *
 * @param n Number of lines.
 */
char *
log_get_last_n_lines (
  Log * self,
  int   n);

/**
 * Initializes logging to a file.
 *
 * This can be called from any thread.
 */
void
log_init_with_file (
  Log * self);

/**
 * Initializes basic logging.
 *
 * This can be called from any thread.
 */
void
log_init (
  Log * self);

/**
 * @}
 */

#endif
