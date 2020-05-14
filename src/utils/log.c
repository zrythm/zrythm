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

#include "utils/mpmc_queue.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/object_pool.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#define MESSAGES_MAX 160000

typedef struct LogEvent
{
  char *         message;
  GLogLevelFlags log_level;
} LogEvent;

/**
 * Write a log message to the log file and to each
 * buffer.
 */
static int
write_str (
  Log *          self,
  GLogLevelFlags log_level,
  char *         str)
{
  /* write to file */
  g_fprintf (
    self->logfile, "%s\n", str);
  fflush (self->logfile);

  /* write to each buffer */
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter (
    self->messages_buf, &iter);
  gtk_text_buffer_insert (
    self->messages_buf, &iter, str, -1);
  gtk_text_buffer_get_end_iter (
    self->messages_buf, &iter);
  gtk_text_buffer_insert (
    self->messages_buf, &iter, "\n", -1);

  return 0;
}

/**
 * Idle callback.
 */
int
log_idle_cb (
  Log * self)
{
  if (!self || !self->mqueue)
    return G_SOURCE_CONTINUE;

  /* write queued messages */
  LogEvent * ev;
  while (
    mpmc_queue_dequeue (
      self->mqueue, (void *) &ev))
    {
      write_str (self, ev->log_level, ev->message);
      g_free (ev->message);
      ev->message = NULL;
      object_pool_return (
        LOG->obj_pool, ev);
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Log writer.
 *
 * If a message is logged from the GTK thread,
 * the message is written immediately, otherwise it
 * is saved to the queue.
 *
 * The queue is only popped when there is a new
 * message in the GTK thread, so the messages will
 * stay in the queue until then.
 */
static GLogWriterOutput
log_writer (
  GLogLevelFlags log_level,
  const GLogField *fields,
  gsize n_fields,
  Log * self)
{
  char * str =
    g_log_writer_format_fields (
      log_level, fields, n_fields, 0);

  /* queue the message */
  LogEvent * ev =
    (LogEvent *)
    object_pool_get (LOG->obj_pool);
  ev->log_level = log_level;
  ev->message = str;
  mpmc_queue_push_back (
    self->mqueue, (void *) ev);

  /* call the default log writer */
  return
    g_log_writer_default (
      log_level, fields, n_fields, self);
}

/**
 * Initializes logging to a file.
 *
 * This must be called from the GTK thread.
 */
void
log_init_writer_idle (
  Log * self)
{
  g_timeout_add_seconds (
    3, (GSourceFunc) log_idle_cb, self);
}

static void *
create_log_event_obj (void)
{
  return calloc (1, sizeof (LogEvent));
}

/**
 * Returns the last \ref n lines as a newly
 * allocated string.
 *
 * @param n Number of lines.
 */
char *
log_get_last_n_lines (
  Log * self,
  int   n)
{
  int total_lines =
    gtk_text_buffer_get_line_count (
      self->messages_buf);
  GtkTextIter start_iter;
  gtk_text_buffer_get_iter_at_line_index (
    self->messages_buf, &start_iter,
    MAX (total_lines - n, 0), 0);
  GtkTextIter end_iter;
  gtk_text_buffer_get_end_iter (
    self->messages_buf, &end_iter);

  return
    gtk_text_buffer_get_text (
      self->messages_buf, &start_iter,
      &end_iter, false);
}

/**
 * Initializes logging to a file.
 *
 * This can be called from any thread.
 */
void
log_init (
  Log * self)
{
  /* open file to write to */
  GDateTime * datetime =
    g_date_time_new_now_local ();
  char * str_datetime =
    g_date_time_format (
      datetime,
      "%F_%H-%M-%S");
  char * user_log_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_LOG);
  char * str =
    g_strdup_printf (
      "%s%slog_%s.log",
      user_log_dir,
      G_DIR_SEPARATOR_S,
      str_datetime);
  io_mkdir (user_log_dir);
  self->logfile = g_fopen (str, "a");
  g_return_if_fail (self->logfile);
  g_free (user_log_dir);
  g_free (str);
  g_free (str_datetime);
  g_date_time_unref (datetime);

  /* init buffers */
  self->messages_buf = gtk_text_buffer_new (NULL);

  /* init the object pool for log events */
  self->obj_pool =
    object_pool_new (
      create_log_event_obj, MESSAGES_MAX);

  /* init the message queue */
  self->mqueue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->mqueue,
    (size_t) MESSAGES_MAX * sizeof (char *));

  g_log_set_writer_func (
    (GLogWriterFunc) log_writer, self, NULL);
}
