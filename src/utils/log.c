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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#ifdef _WOE32
#include <process.h>
#endif

#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#define MESSAGES_MAX 160000

/* string size big enough to hold level prefix */
#define	STRING_BUFFER_SIZE	\
  (FORMAT_UNSIGNED_BUFSIZE + 32)

#define	ALERT_LEVELS		(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)

#define CHAR_IS_SAFE(wc) (!((wc < 0x20 && wc != '\t' && wc != '\n' && wc != '\r') || \
			    (wc == 0x7f) || \
			    (wc >= 0x80 && wc < 0xa0)))

/* For a radix of 8 we need at most 3 output bytes for 1 input
 * byte. Additionally we might need up to 2 output bytes for the
 * readix prefix and 1 byte for the trailing NULL.
 */
#define FORMAT_UNSIGNED_BUFSIZE ((GLIB_SIZEOF_LONG * 3) + 3)

static GLogLevelFlags g_log_msg_prefix = G_LOG_LEVEL_ERROR | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_DEBUG;

#ifdef _WOE32
static gboolean win32_keep_fatal_message = FALSE;
#endif

typedef struct LogEvent
{
  char *         message;
  GLogLevelFlags log_level;
} LogEvent;

#ifndef HAVE_G_GET_CONSOLE_CHARSET
static gboolean
g_get_console_charset (const char ** charset)
{
  return g_get_charset (charset);
}
#endif

/**
 * @note from GLib.
 */
static void
escape_string (GString *string)
{
  const char *p = string->str;
  gunichar wc;

  while (p < string->str + string->len)
    {
      gboolean safe;

      wc = g_utf8_get_char_validated (p, -1);
      if (wc == (gunichar)-1 || wc == (gunichar)-2)
	{
	  gchar *tmp;
	  guint pos;

	  pos = p - string->str;

	  /* Emit invalid UTF-8 as hex escapes
           */
	  tmp = g_strdup_printf ("\\x%02x", (guint)(guchar)*p);
	  g_string_erase (string, pos, 1);
	  g_string_insert (string, pos, tmp);

	  p = string->str + (pos + 4); /* Skip over escape sequence */

	  g_free (tmp);
	  continue;
	}
      if (wc == '\r')
	{
	  safe = *(p + 1) == '\n';
	}
      else
	{
	  safe = CHAR_IS_SAFE (wc);
	}

      if (!safe)
	{
	  gchar *tmp;
	  guint pos;

	  pos = p - string->str;

	  /* Largest char we escape is 0x0a, so we don't have to worry
	   * about 8-digit \Uxxxxyyyy
	   */
	  tmp = g_strdup_printf ("\\u%04x", wc);
	  g_string_erase (string, pos, g_utf8_next_char (p) - p);
	  g_string_insert (string, pos, tmp);
	  g_free (tmp);

	  p = string->str + (pos + 6); /* Skip over escape sequence */
	}
      else
	p = g_utf8_next_char (p);
    }
}

/**
 * @note from GLib.
 */
static void
format_unsigned (gchar  *buf,
		 gulong  num,
		 guint   radix)
{
  gulong tmp;
  gchar c;
  gint i, n;

  /* we may not call _any_ GLib functions here (or macros like g_return_if_fail()) */

  if (radix != 8 && radix != 10 && radix != 16)
    {
      *buf = '\000';
      return;
    }

  if (!num)
    {
      *buf++ = '0';
      *buf = '\000';
      return;
    }

  if (radix == 16)
    {
      *buf++ = '0';
      *buf++ = 'x';
    }
  else if (radix == 8)
    {
      *buf++ = '0';
    }

  n = 0;
  tmp = num;
  while (tmp)
    {
      tmp /= radix;
      n++;
    }

  i = n;

  /* Again we can't use g_assert; actually this check should _never_ fail. */
  if (n > FORMAT_UNSIGNED_BUFSIZE - 3)
    {
      *buf = '\000';
      return;
    }

  while (num)
    {
      i--;
      c = (num % radix);
      if (c < 10)
	buf[i] = c + '0';
      else
	buf[i] = c + 'a' - 10;
      num /= radix;
    }

  buf[n] = '\000';
}

/**
 * @note from GLib.
 */
static FILE *
log_level_to_file (GLogLevelFlags log_level)
{
  if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL |
                   G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE))
    return stderr;
  else
    return stdout;
}

/**
 * @note from GLib.
 */
static const gchar *
log_level_to_color (GLogLevelFlags log_level,
                    gboolean       use_color)
{
  /* we may not call _any_ GLib functions here */

  if (!use_color)
    return "";

  if (log_level & G_LOG_LEVEL_ERROR)
    return "\033[1;31m"; /* red */
  else if (log_level & G_LOG_LEVEL_CRITICAL)
    return "\033[1;35m"; /* magenta */
  else if (log_level & G_LOG_LEVEL_WARNING)
    return "\033[1;33m"; /* yellow */
  else if (log_level & G_LOG_LEVEL_MESSAGE)
    return "\033[1;32m"; /* green */
  else if (log_level & G_LOG_LEVEL_INFO)
    return "\033[1;32m"; /* green */
  else if (log_level & G_LOG_LEVEL_DEBUG)
    return "\033[1;32m"; /* green */

  /* No color for custom log levels. */
  return "";
}

/**
 * @note from GLib.
 */
static const gchar *
color_reset (gboolean use_color)
{
  /* we may not call _any_ GLib functions here */

  if (!use_color)
    return "";

  return "\033[0m";
}

/**
 * @note from GLib.
 */
static FILE *
mklevel_prefix (
  gchar          level_prefix[STRING_BUFFER_SIZE],
  GLogLevelFlags log_level,
  gboolean       use_color)
{
  gboolean to_stdout = TRUE;

  strcpy (
    level_prefix,
    log_level_to_color (log_level, use_color));

  switch (log_level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR:
      strcat (level_prefix, "ERROR");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_CRITICAL:
      strcat (level_prefix, "CRITICAL");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_WARNING:
      strcat (level_prefix, "WARNING");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_MESSAGE:
      strcat (level_prefix, "Message");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_INFO:
      strcat (level_prefix, "INFO");
      break;
    case G_LOG_LEVEL_DEBUG:
      strcat (level_prefix, "DEBUG");
      break;
    default:
      if (log_level)
	{
	  strcat (level_prefix, "LOG-");
	  format_unsigned (
      level_prefix + 4,
      (gulong) (log_level & G_LOG_LEVEL_MASK), 16);
	}
      else
	strcat (level_prefix, "LOG");
      break;
    }

  strcat (level_prefix, color_reset (use_color));

  if (log_level & G_LOG_FLAG_RECURSION)
    strcat (level_prefix, " (recursed)");
  if (log_level & ALERT_LEVELS)
    strcat (level_prefix, " **");

#ifdef G_OS_WIN32
  if ((log_level & G_LOG_FLAG_FATAL) != 0 && !g_test_initialized ())
    win32_keep_fatal_message = TRUE;
#endif
  return to_stdout ? stdout : stderr;
}

/**
 * @note from GLib.
 */
static gchar*
strdup_convert (const gchar *string,
		const gchar *charset)
{
  if (!g_utf8_validate (string, -1, NULL))
    {
      GString *gstring = g_string_new ("[Invalid UTF-8] ");
      guchar *p;

      for (p = (guchar *)string; *p; p++)
	{
	  if (CHAR_IS_SAFE(*p) &&
	      !(*p == '\r' && *(p + 1) != '\n') &&
	      *p < 0x80)
	    g_string_append_c (gstring, (char) *p);
	  else
	    g_string_append_printf (gstring, "\\x%02x", (guint)(guchar)*p);
	}

      return g_string_free (gstring, FALSE);
    }
  else
    {
      GError *err = NULL;

      gchar *result = g_convert_with_fallback (string, -1, charset, "UTF-8", "?", NULL, NULL, &err);
      if (result)
	return result;
      else
	{
	  /* Not thread-safe, but doesn't matter if we print the warning twice
	   */
	  static gboolean warned = FALSE;
	  if (!warned)
	    {
	      warned = TRUE;
	      g_fprintf (stderr, "GLib: Cannot convert message: %s\n", err->message);
	    }
	  g_error_free (err);

	  return g_strdup (string);
	}
    }
}

/**
 * @note from GLib.
 */
static gchar *
log_writer_format_fields (
  GLogLevelFlags   log_level,
  const GLogField *fields,
  gsize            n_fields,
  gboolean         use_color)
{
  gsize i;
  const gchar *message = NULL;
  const gchar *log_domain = NULL;
  const gchar *line = NULL;
  const gchar *func = NULL;
  /*const gchar *file = NULL;*/
  gchar level_prefix[STRING_BUFFER_SIZE];
  GString *gstring;
  gint64 now;
  time_t now_secs;
  struct tm *now_tm;
  gchar time_buf[128];

  /* Extract some common fields. */
  for (i = 0; (message == NULL || log_domain == NULL) && i < n_fields; i++)
    {
      const GLogField *field = &fields[i];

      if (g_strcmp0 (field->key, "MESSAGE") == 0)
        message = field->value;
      else if (g_strcmp0 (field->key, "GLIB_DOMAIN") == 0)
        log_domain = field->value;
      /*else if (g_strcmp0 (field->key, "CODE_FILE") == 0)*/
        /*file = field->value;*/
      else if (g_strcmp0 (field->key, "CODE_FUNC") == 0)
        func = field->value;
      else if (g_strcmp0 (field->key, "CODE_LINE") == 0)
        line = field->value;
    }

  /* Format things. */
  mklevel_prefix (level_prefix, log_level, use_color);

  gstring = g_string_new (NULL);
  if (log_level & ALERT_LEVELS)
    g_string_append (gstring, "\n");
  if (!log_domain)
    g_string_append (gstring, "** ");

  if ((g_log_msg_prefix & (log_level & G_LOG_LEVEL_MASK)) ==
      (log_level & G_LOG_LEVEL_MASK))
    {
      const gchar *prg_name = g_get_prgname ();
#ifdef _WOE32
      gulong pid = (gulong) _getpid ();
#else
      gulong pid = (gulong) getpid ();
#endif

      if (prg_name == NULL)
        g_string_append_printf (gstring, "(process:%lu): ", pid);
      else
        g_string_append_printf (gstring, "(%s:%lu): ", prg_name, pid);
    }

  if (log_domain != NULL)
    {
      g_string_append (gstring, log_domain);
      g_string_append_c (gstring, '-');
    }
  g_string_append (gstring, level_prefix);

  g_string_append (gstring, ": ");

  /* Timestamp */
  now = g_get_real_time ();
  now_secs = (time_t) (now / 1000000);
  now_tm = localtime (&now_secs);
  strftime (time_buf, sizeof (time_buf), "%H:%M:%S", now_tm);

  g_string_append_printf (gstring, "%s%s.%03d%s: ",
                          use_color ? "\033[34m" : "",
                          time_buf, (gint) ((now / 1000) % 1000),
                          color_reset (use_color));

  /* EDIT: append file, func and line */
  g_string_append_printf (
    gstring, "%s(%s:%s)%s: ",
    use_color ? "\033[90m" : "",
    func, line,
    color_reset (use_color));

  if (message == NULL)
    {
      g_string_append (gstring, "(NULL) message");
    }
  else
    {
      GString *msg;
      const gchar *charset;

      msg = g_string_new (message);
      escape_string (msg);

      if (g_get_console_charset (&charset))
        {
          /* charset is UTF-8 already */
          g_string_append (gstring, msg->str);
        }
      else
        {
          gchar *lstring = strdup_convert (msg->str, charset);
          g_string_append (gstring, lstring);
          g_free (lstring);
        }

      g_string_free (msg, TRUE);
    }

  return g_string_free (gstring, FALSE);
}

/**
 * @note from GLib.
 */
static GLogWriterOutput
log_writer_standard_streams (
  GLogLevelFlags   log_level,
  const GLogField *fields,
  gsize            n_fields,
  gpointer         user_data)
{
  FILE *stream;
  gchar *out = NULL;  /* in the current locale’s character set */

  g_return_val_if_fail (fields != NULL, G_LOG_WRITER_UNHANDLED);
  g_return_val_if_fail (n_fields > 0, G_LOG_WRITER_UNHANDLED);

  stream = log_level_to_file (log_level);
  if (!stream || fileno (stream) < 0)
    return G_LOG_WRITER_UNHANDLED;

  out = log_writer_format_fields (log_level, fields, n_fields,
                                    g_log_writer_supports_color (fileno (stream)));
  g_fprintf (stream, "%s\n", out);
  fflush (stream);
  g_free (out);

  return G_LOG_WRITER_HANDLED;
}

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
    log_writer_format_fields (
      log_level, fields, n_fields, F_NO_USE_COLOR);

  if (self->initialized)
    {
      /* queue the message */
      LogEvent * ev =
        (LogEvent *)
        object_pool_get (self->obj_pool);
      ev->log_level = log_level;
      ev->message = str;
      mpmc_queue_push_back (
        self->mqueue, (void *) ev);
    }
  else
    {
#ifdef _WOE32
      /* log file not ready yet, log to console */
      fprintf (stderr, "%s\n", str);
#endif
    }

  /* call the default log writer */
  return
    log_writer_standard_streams (
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
  self->writer_source_id =
    g_timeout_add_seconds (
      3, (GSourceFunc) log_idle_cb, self);
}

static void *
create_log_event_obj (void)
{
  return calloc (1, sizeof (LogEvent));
}

static void
free_log_event_obj (
  LogEvent * ev)
{
  g_free_and_null (ev->message);

  object_zero_and_free (ev);
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
  if (!GTK_IS_TEXT_BUFFER (
        self->messages_buf))
    return NULL;

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
log_init_with_file (
  Log * self)
{
  /* open file to write to */
  char * str_datetime =
    datetime_get_for_filename ();
  char * user_log_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_LOG);
  char * str =
    g_strdup_printf (
      "%s%slog_%s.log",
      user_log_dir,
      G_DIR_SEPARATOR_S,
      str_datetime);
  io_mkdir (user_log_dir);
  self->logfile = fopen (str, "a");
  g_return_if_fail (self->logfile);
  g_free (user_log_dir);
  g_free (str);
  g_free (str_datetime);

  /* init buffers */
  self->messages_buf = gtk_text_buffer_new (NULL);

  /* init the object pool for log events */
  self->obj_pool =
    object_pool_new (
      create_log_event_obj,
      (ObjectFreeFunc) free_log_event_obj,
      MESSAGES_MAX);

  /* init the message queue */
  self->mqueue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->mqueue,
    (size_t) MESSAGES_MAX * sizeof (char *));

  self->initialized = true;
}

/**
 * Creates the logger and sets the writer func.
 *
 * This can be called from any thread.
 */
Log *
log_new (void)
{
  Log * self = object_new (Log);

  g_log_set_writer_func (
    (GLogWriterFunc) log_writer, self, NULL);

  return self;
}

/**
 * Stops logging and frees any allocated memory.
 */
void
log_free (
  Log * self)
{
  g_message ("%s: Tearing down...", __func__);

  /* remove source func */
  g_source_remove (self->writer_source_id);

  /* clear the queue */
  log_idle_cb (self);

  /* set back default log handler */
  g_log_set_writer_func (
    g_log_writer_default, NULL, NULL);

  /* close log file */
  fclose (self->logfile);

  /* free children */
  object_free_w_func_and_null (
    object_pool_free, self->obj_pool);
  object_free_w_func_and_null (
    mpmc_queue_free, self->mqueue);
  g_object_unref_and_null (self->messages_buf);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
