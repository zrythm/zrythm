/*
 * Copyright (C) 2019 Alexandros Theodotou <alex@zrythm.org>
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

#include "utils/log.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

static FILE * logfile;

/**
 * Log writer.
 */
static GLogWriterOutput
log_writer (
  GLogLevelFlags log_level,
  const GLogField *fields,
  gsize n_fields,
  gpointer user_data)
{
  /* write to file */
  g_fprintf (
    logfile, "%s\n",
    g_log_writer_format_fields (
      log_level, fields, n_fields, 0));
  fflush (logfile);

  /* call the default log writer */
  return g_log_writer_default (
    log_level,
    fields,
    n_fields,
    user_data);
}

/**
 * Initializes logging to a file.
 */
void
log_init ()
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
      "%s%slog_%s.txt",
      ZRYTHM->log_dir,
      G_DIR_SEPARATOR_S,
      str_datetime);
  logfile =
    g_fopen (str, "a");
  g_return_if_fail (logfile);
  g_free (str);
  g_free (str_datetime);
  g_date_time_unref (datetime);

  g_log_set_writer_func (
    log_writer,
    NULL,
    NULL);
}
