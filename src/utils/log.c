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

#include "utils/log.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

/**
 * Log writer.
 */
static GLogWriterOutput
log_writer (
  GLogLevelFlags log_level,
  const GLogField *fields,
  gsize n_fields,
  Log * self)
{
  /* write to file */
  char * str =
    g_log_writer_format_fields (
      log_level, fields, n_fields, 0);
  g_fprintf (
    self->logfile, "%s\n", str);
  fflush (self->logfile);

  if (g_thread_self () == ZRYTHM->gtk_thread)
    {
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
    }
  else
    {
      /* TODO queue the message and then unqueue it
       * above when in the gtk thread */
    }

  g_free (str);

  /* call the default log writer */
  return
    g_log_writer_default (
      log_level, fields, n_fields, self);
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
  char * str =
    g_strdup_printf (
      "%s%slog_%s",
      ZRYTHM->log_dir,
      G_DIR_SEPARATOR_S,
      str_datetime);
  self->logfile =
    g_fopen (str, "a");
  g_return_if_fail (self->logfile);
  g_free (str);
  g_free (str_datetime);
  g_date_time_unref (datetime);

  /* init buffers */
  self->messages_buf = gtk_text_buffer_new (NULL);

  g_log_set_writer_func (
    (GLogWriterFunc) log_writer, self, NULL);
}
