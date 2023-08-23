// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * ---
 */

/* for dprintf */
#define _GNU_SOURCE

#include "zrythm-config.h"

#include <stdio.h>

#ifdef _WOE32
#  include <process.h>
#endif

#include "dsp/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/dialogs/bug_report_dialog.h"
#include "gui/widgets/header.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/backtrace.h"
#include "utils/datetime.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#ifdef HAVE_VALGRIND
#  include <valgrind/valgrind.h>
#else
#  define RUNNING_ON_VALGRIND 0
#endif
#include <zstd.h>

typedef enum
{
  Z_UTILS_LOG_ERROR_FAILED,
} ZUtilsLogError;

#define Z_UTILS_LOG_ERROR z_utils_log_error_quark ()
GQuark
z_utils_log_error_quark (void);
G_DEFINE_QUARK (
  z - utils - log - error - quark,
  z_utils_log_error)

/** This is declared extern in log.h. */
Log * zlog = NULL;

#define MESSAGES_MAX 160000

/* string size big enough to hold level prefix */
#define STRING_BUFFER_SIZE (FORMAT_UNSIGNED_BUFSIZE + 32)

#define ALERT_LEVELS \
  (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL \
   | G_LOG_LEVEL_WARNING)

#define CHAR_IS_SAFE(wc) \
  (!( \
    (wc < 0x20 && wc != '\t' && wc != '\n' && wc != '\r') \
    || (wc == 0x7f) || (wc >= 0x80 && wc < 0xa0)))

/* For a radix of 8 we need at most 3 output bytes for 1 input
 * byte. Additionally we might need up to 2 output bytes for the
 * readix prefix and 1 byte for the trailing NULL.
 */
#define FORMAT_UNSIGNED_BUFSIZE ((GLIB_SIZEOF_LONG * 3) + 3)

/* these are emitted by the default log handler */
#define DEFAULT_LEVELS \
  (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL \
   | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE)
/* these are filtered by G_MESSAGES_DEBUG by the default log handler */
#define INFO_LEVELS (G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)

static GLogLevelFlags g_log_msg_prefix =
  G_LOG_LEVEL_ERROR | G_LOG_LEVEL_WARNING
  | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_DEBUG;

#ifdef _WOE32
static gboolean win32_keep_fatal_message = FALSE;
static gchar    fatal_msg_buf[1000] =
  "Unspecified fatal error encountered, aborting.";
#endif

static GLogLevelFlags log_always_fatal = G_LOG_FATAL_MASK;

static char * tmp_log_file = NULL;

/**
 * Whether to use the default log writer (eg after the log
 * object has been free'd.
 *
 * Used by the log writer.
 */
static bool use_default_log_writer = false;

typedef struct LogEvent
{
  char * message;

  /** Backtrace, if warning or critical. */
  char *         backtrace;
  GLogLevelFlags log_level;

  /** Whether the log event is from the "zrythm" log domain. */
  bool is_zrythm_domain;
} LogEvent;

static void
_log_abort (gboolean breakpoint)
{
  gboolean debugger_present;

  if (g_test_subprocess ())
    {
      /* If this is a test case subprocess then it probably caused
       * this error message on purpose, so just exit() rather than
       * abort()ing, to avoid triggering any system crash-reporting
       * daemon.
       */
      _exit (1);
    }

#ifdef G_OS_WIN32
  debugger_present = IsDebuggerPresent ();
#else
  /* Assume GDB is attached. */
  debugger_present = TRUE;
#endif /* !G_OS_WIN32 */

  /*g_warn_if_reached ();*/

  if (debugger_present && breakpoint)
    G_BREAKPOINT ();
  else
    g_abort ();
}

/**
 * @note from GLib.
 */
static void
escape_string (GString * string)
{
  const char * p = string->str;
  gunichar     wc;

  while (p < string->str + string->len)
    {
      gboolean safe;

      wc = g_utf8_get_char_validated (p, -1);
      if (wc == (gunichar) -1 || wc == (gunichar) -2)
        {
          gchar * tmp;
          guint   pos;

          pos = p - string->str;

          /* Emit invalid UTF-8 as hex escapes
                 */
          tmp =
            g_strdup_printf ("\\x%02x", (guint) (guchar) *p);
          g_string_erase (string, pos, 1);
          g_string_insert (string, pos, tmp);

          p = string->str
              + (pos + 4); /* Skip over escape sequence */

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
          gchar * tmp;
          guint   pos;

          pos = p - string->str;

          /* Largest char we escape is 0x0a, so we don't have to worry
           * about 8-digit \Uxxxxyyyy
           */
          tmp = g_strdup_printf ("\\u%04x", wc);
          g_string_erase (
            string, pos, g_utf8_next_char (p) - p);
          g_string_insert (string, pos, tmp);
          g_free (tmp);

          p = string->str
              + (pos + 6); /* Skip over escape sequence */
        }
      else
        p = g_utf8_next_char (p);
    }
}

/**
 * @note from GLib.
 */
static void
format_unsigned (gchar * buf, gulong num, guint radix)
{
  gulong tmp;
  gchar  c;
  gint   i, n;

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
  if (
    log_level
    & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE))
    return stderr;
  else
    return stdout;
}

/**
 * @note from GLib.
 */
static const gchar *
log_level_to_color (GLogLevelFlags log_level, gboolean use_color)
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
    level_prefix, log_level_to_color (log_level, use_color));

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
  if (
    (log_level & G_LOG_FLAG_FATAL) != 0
    && !g_test_initialized ())
    win32_keep_fatal_message = TRUE;
#endif
  return to_stdout ? stdout : stderr;
}

/**
 * @note from GLib.
 */
static gchar *
strdup_convert (const gchar * string, const gchar * charset)
{
  if (!g_utf8_validate (string, -1, NULL))
    {
      GString * gstring = g_string_new ("[Invalid UTF-8] ");
      guchar *  p;

      for (p = (guchar *) string; *p; p++)
        {
          if (
            CHAR_IS_SAFE (*p)
            && !(*p == '\r' && *(p + 1) != '\n') && *p < 0x80)
            g_string_append_c (gstring, (char) *p);
          else
            g_string_append_printf (
              gstring, "\\x%02x", (guint) (guchar) *p);
        }

      return g_string_free (gstring, FALSE);
    }
  else
    {
      GError * err = NULL;

      gchar * result = g_convert_with_fallback (
        string, -1, charset, "UTF-8", "?", NULL, NULL, &err);
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
              g_fprintf (
                stderr, "GLib: Cannot convert message: %s\n",
                err->message);
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
  GLogLevelFlags    log_level,
  const GLogField * fields,
  gsize             n_fields,
  gboolean          use_color)
{
  gsize         i;
  const gchar * message = NULL;
  const gchar * log_domain = NULL;
  const gchar * line = NULL;
  const gchar * func = NULL;
  /*const gchar *file = NULL;*/
  gchar       level_prefix[STRING_BUFFER_SIZE];
  GString *   gstring;
  gint64      now;
  time_t      now_secs;
  struct tm * now_tm;
  gchar       time_buf[128];

  /* Extract some common fields. */
  for (i = 0;
       (message == NULL || log_domain == NULL) && i < n_fields;
       i++)
    {
      const GLogField * field = &fields[i];

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

  if (
    (g_log_msg_prefix & (log_level & G_LOG_LEVEL_MASK))
    == (log_level & G_LOG_LEVEL_MASK))
    {
      const gchar * prg_name = g_get_prgname ();
#ifdef _WOE32
      gulong pid = (gulong) _getpid ();
#else
      gulong pid = (gulong) getpid ();
#endif

      if (prg_name == NULL)
        g_string_append_printf (
          gstring, "(process:%lu): ", pid);
      else
        g_string_append_printf (
          gstring, "(%s:%lu): ", prg_name, pid);
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

  g_string_append_printf (
    gstring, "%s%s.%03d%s: ", use_color ? "\033[34m" : "",
    time_buf, (gint) ((now / 1000) % 1000),
    color_reset (use_color));

  /* EDIT: append file, func and line */
  g_string_append_printf (
    gstring, "%s(%s:%s)%s: ", use_color ? "\033[90m" : "",
    func ? func : "?", line ? line : "?",
    color_reset (use_color));

  if (message == NULL)
    {
      g_string_append (gstring, "(NULL) message");
    }
  else
    {
      GString * msg = g_string_new (message);
      escape_string (msg);

      const gchar * charset;
      if (g_get_console_charset (&charset))
        {
          /* charset is UTF-8 already */
          g_string_append (gstring, msg->str);
        }
      else
        {
          gchar * lstring = strdup_convert (msg->str, charset);
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
  GLogLevelFlags    log_level,
  const GLogField * fields,
  gsize             n_fields,
  gpointer          user_data)
{
  FILE *  stream;
  gchar * out =
    NULL; /* in the current locale’s character set */

  g_return_val_if_fail (
    fields != NULL, G_LOG_WRITER_UNHANDLED);
  g_return_val_if_fail (n_fields > 0, G_LOG_WRITER_UNHANDLED);

  stream = log_level_to_file (log_level);
  if (!stream || fileno (stream) < 0)
    return G_LOG_WRITER_UNHANDLED;

  out = log_writer_format_fields (
    log_level, fields, n_fields,
    g_log_writer_supports_color (fileno (stream)));
  g_fprintf (stream, "%s\n", out);
  fflush (stream);
  g_free (out);

  return G_LOG_WRITER_HANDLED;
}

static bool
need_backtrace (Log * self, const LogEvent * const ev)
{
  /* cooldown time in to avoid excessive sequential
   * backtraces - we are only interested in the
   * first one in almost every case */
  /* 16 seconds */
#define BT_COOLDOWN_TIME (16 * 1000 * 1000)

  gint64 time_now = g_get_monotonic_time ();

  bool ret =
    (time_now - self->last_bt_time > BT_COOLDOWN_TIME)
    && ev->log_level == G_LOG_LEVEL_CRITICAL
    && ev->is_zrythm_domain
    && !string_contains_substr (
      ev->message,
      "assertion 'size >= 0' failed in "
      "GtkScrollbar")
    && !string_contains_substr (
      ev->message,
      "assertion 'size >= 0' failed in "
      "GtkNotebook")
    && !string_contains_substr (
      ev->message,
      "gtk_window_set_titlebar() called on a "
      "realized window")
    && !string_contains_substr (
      ev->message, "attempt to allocate widget")
    && !string_contains_substr (
      ev->message, "Theme file for default has no directories")
    && !string_contains_substr (
      ev->message,
      "gsk_render_node_unref: assertion 'GSK_IS_RENDER_NODE (node)' failed")
    && !string_contains_substr (
      ev->message, "A floating object GtkAdjustment")
    && !string_contains_substr (
      ev->message,
      "gtk_css_node_insert_after: assertion 'previous_sibling == NULL || previous_sibling->parent == parent' failed")
    && !string_contains_substr (
      ev->message,
      "you are running a non-free operating system")
    && !string_contains_substr (
      ev->message, "Allocation height too small")
    && !string_contains_substr (
      ev->message, "Allocation width too small")
    && !string_contains_substr (
      ev->message, "GDK_DROP_STATE_NONE")
    &&
    /* this happens with portals when opening
     * the native file chooser (either flatpak
     * builds or when using GTK_USE_PORTAL=1
     * TODO: only do this if using portals, find
     * a way to check if using portals via code */
    !string_contains_substr (
      ev->message,
      "g_variant_new_string: assertion "
      "'string != NULL' failed")
    &&
    /* this happens in the first run dialog and
     * bug report dialog */
    !string_contains_substr (
      ev->message,
      "gtk_box_append: assertion "
      "'gtk_widget_get_parent (child) == NULL' "
      "failed")
    && !string_contains_substr (
      ev->message,
      "assertion 'self->drop == "
      "gdk_dnd_event_get_drop (event)'")
    &&
    /* libpanel */
    !string_contains_substr (
      ev->message,
      "gdk_drop_status: assertion 'gdk_drag_action_is_unique (preferred)' failed")
    &&
    /* gtk error after last update */
    !string_contains_substr (
      ev->message, "reports a minimum width of ")
    &&
    /* user's system is broken - avoid bug reports */
    !string_contains_substr (
      ev->message,
      "Unable to register the application: GDBus.Error:org.freedesktop.DBus.Error.NoReply: Message recipient disconnected from message bus without replying")
    &&
    /* wireplumber bug */
    !string_contains_substr (
      ev->message,
      "Failed to set scheduler settings: Operation not permitted");

  if (ret)
    self->last_bt_time = time_now;

  return ret;
}

/**
 * Write a log message to the log file and to each
 * buffer.
 */
static int
write_str (Log * self, GLogLevelFlags log_level, char * str)
{
  /* write to file */
  if (self->logfile)
    {
      g_fprintf (self->logfile, "%s\n", str);
      fflush (self->logfile);
    }
#if 0
  else if (self->logfd > -1)
    {
#  ifdef __linux__
      dprintf (
        self->logfd, "%s\n", str);
#  endif

#  ifdef _WOE32
      FlushFileBuffers (
        (HANDLE) self->logfd);
#  else
      fsync (self->logfd);
#  endif
    }
#endif

#if 0
  /* write to each buffer */
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter (
    self->messages_buf, &iter);
  if (g_utf8_validate (str, -1, NULL))
    {
      gtk_text_buffer_insert (
        self->messages_buf, &iter, str, -1);
    }
  else
    {
      g_message (
        "Attempted to add a non-UTF8 string to "
        "the log file: %s", str);
      gtk_text_buffer_insert (
        self->messages_buf, &iter,
        "Non-UTF8 string!", -1);
    }
  gtk_text_buffer_get_end_iter (
    self->messages_buf, &iter);
  gtk_text_buffer_insert (
    self->messages_buf, &iter, "\n", -1);
#endif

  return 0;
}

/**
 * Idle callback.
 */
int
log_idle_cb (Log * self)
{
  if (!self->mqueue)
    return G_SOURCE_CONTINUE;

  /* write queued messages */
  LogEvent * ev;
  while (mpmc_queue_dequeue (self->mqueue, (void *) &ev))
    {
      write_str (self, ev->log_level, ev->message);

      if (ev->backtrace)
        {
          if (!ZRYTHM || ZRYTHM_TESTING)
            {
              g_message ("Backtrace: %s", ev->backtrace);
            }

          if (
            ev->log_level == G_LOG_LEVEL_CRITICAL
            && ZRYTHM_HAVE_UI
            && !zrythm_app->bug_report_dialog)
            {
              char msg[500];
              sprintf (
                msg, _ ("%s has encountered an error\n"),
                PROGRAM_NAME);
              zrythm_app
                ->bug_report_dialog = bug_report_dialog_new (
                MAIN_WINDOW ? GTK_WINDOW (MAIN_WINDOW) : NULL,
                msg, ev->backtrace, false);
              gtk_window_present (
                GTK_WINDOW (zrythm_app->bug_report_dialog));
            }

          /* write the backtrace to the log after
           * showing the popup (if any) */
          if (self->logfile)
            {
              g_fprintf (self->logfile, "%s\n", ev->backtrace);
              fflush (self->logfile);
            }
        } /* endif ev->backtrace */

      if (
        ev->log_level == G_LOG_LEVEL_WARNING && ZRYTHM_HAVE_UI
        && MAIN_WINDOW && MW_HEADER
        && !MAIN_WINDOW->log_has_pending_warnings)
        {
          MAIN_WINDOW->log_has_pending_warnings = true;
          EVENTS_PUSH (ET_LOG_WARNING_STATE_CHANGED, NULL);
        }

      g_free_and_null (ev->backtrace);
      g_free_and_null (ev->message);
      object_pool_return (LOG->obj_pool, ev);
    }

  return G_SOURCE_CONTINUE;
}

static gboolean
log_is_old_api (const GLogField * fields, gsize n_fields)
{
  return (
    n_fields >= 1
    && g_strcmp0 (fields[0].key, "GLIB_OLD_LOG_API") == 0
    && g_strcmp0 (fields[0].value, "1") == 0);
}

static GLogWriterOutput
log_writer_default_custom (
  GLogLevelFlags    log_level,
  const GLogField * fields,
  gsize             n_fields,
  gpointer          user_data)
{
  Log * self = (Log *) user_data;

  static gsize    initialized = 0;
  static gboolean stderr_is_journal = FALSE;

  g_return_val_if_fail (
    fields != NULL, G_LOG_WRITER_UNHANDLED);
  g_return_val_if_fail (n_fields > 0, G_LOG_WRITER_UNHANDLED);

  /* Disable debug message output unless specified in G_MESSAGES_DEBUG. */
  if (
    !(log_level & DEFAULT_LEVELS)
    && !(log_level >> G_LOG_LEVEL_USER_SHIFT))
    {
      const gchar * log_domain = NULL;
      gsize         i;

      if (
        (log_level & INFO_LEVELS) == 0
        || self->log_domains == NULL)
        return G_LOG_WRITER_HANDLED;

      for (i = 0; i < n_fields; i++)
        {
          if (g_strcmp0 (fields[i].key, "GLIB_DOMAIN") == 0)
            {
              log_domain = fields[i].value;
              break;
            }
        }

      if (
        strcmp (self->log_domains, "all") != 0
        && (log_domain == NULL || !strstr (self->log_domains, log_domain)))
        return G_LOG_WRITER_HANDLED;
    }

  /* Mark messages as fatal if they have a level set in
   * g_log_set_always_fatal().
   */
  if (
    (log_level & log_always_fatal)
    && !log_is_old_api (fields, n_fields))
    log_level |= G_LOG_FLAG_FATAL;

  /* Try logging to the systemd journal as first choice. */
  if (g_once_init_enter (&initialized))
    {
      stderr_is_journal =
        g_log_writer_is_journald (fileno (stderr));
      g_once_init_leave (&initialized, TRUE);
    }

  if (
    stderr_is_journal
    && g_log_writer_journald (
         log_level, fields, n_fields, user_data)
         == G_LOG_WRITER_HANDLED)
    goto handled;

  /* FIXME: Add support for the Windows log. */

  if (
    log_writer_standard_streams (
      log_level, fields, n_fields, user_data)
    == G_LOG_WRITER_HANDLED)
    goto handled;

  return G_LOG_WRITER_UNHANDLED;

handled:
  /* Abort if the message was fatal. */
  if (log_level & G_LOG_FLAG_FATAL)
    {
#ifdef G_OS_WIN32
      if (!g_test_initialized ())
        {
          gchar * locale_msg = NULL;

          locale_msg = g_locale_from_utf8 (
            fatal_msg_buf, -1, NULL, NULL, NULL);
          MessageBox (
            NULL, locale_msg, NULL,
            MB_ICONERROR | MB_SETFOREGROUND);
          g_free (locale_msg);
        }
#endif /* !G_OS_WIN32 */

      _log_abort (!(log_level & G_LOG_FLAG_RECURSION));
    }

  return G_LOG_WRITER_HANDLED;
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
  GLogLevelFlags    log_level,
  const GLogField * fields,
  gsize             n_fields,
  Log *             self)
{
  if (use_default_log_writer)
    {
      return g_log_writer_default (
        log_level, fields, n_fields, NULL);
    }

  char * str = log_writer_format_fields (
    log_level, fields, n_fields, F_NO_USE_COLOR);

  if (self->initialized)
    {
      /* queue the message */
      LogEvent * ev =
        (LogEvent *) object_pool_get (self->obj_pool);
      ev->log_level = log_level;
      ev->message = str;

      ev->is_zrythm_domain = false;
      for (gsize i = 0; i < n_fields; i++)
        {
          if (
            string_is_equal (fields[i].key, "GLIB_DOMAIN")
            && string_is_equal (
              (const char *) fields[i].value, "zrythm"))
            {
              ev->is_zrythm_domain = true;
              break;
            }
        }

      if (need_backtrace (self, ev) && !RUNNING_ON_VALGRIND)
        {
          ev->backtrace =
            backtrace_get_with_lines ("", 100, true);
        }

      int num_avail_objs =
        object_pool_get_num_available (self->obj_pool);
      if (num_avail_objs < 200)
        {
          log_idle_cb (self);
        }

      mpmc_queue_push_back (self->mqueue, (void *) ev);
    }
  else
    {
#ifdef _WOE32
      /* log file not ready yet, log to console */
      fprintf (stderr, "%s\n", str);
#endif

      /* also log to /tmp */
      if (!tmp_log_file)
        {
          char * datetime = datetime_get_for_filename ();
          char * filename =
            g_strdup_printf ("zrythm_%s.log", datetime);
          const char * tmpdir = g_get_tmp_dir ();
          tmp_log_file =
            g_build_filename (tmpdir, filename, NULL);
          g_free (filename);
          g_free (datetime);
        }
      FILE * file = fopen (tmp_log_file, "a");
      g_return_val_if_fail (file, G_LOG_WRITER_UNHANDLED);
      fprintf (file, "%s\n", str);
      fclose (file);
    }

  /*(void) log_writer_standard_streams;*/

  /* call the default log writer */
  if (ZRYTHM && ZRYTHM_TESTING)
    {
      if (self->use_structured_for_console)
        {
          /* this is because the following doesn't
           * send a trap when executed from non-gtk
           * threads during testing */
          return g_log_writer_default (
            log_level, fields, n_fields, self);
        }
      else
        {
          if (log_level <= self->min_log_level_for_test_console)
            {
              g_log (G_LOG_DOMAIN, log_level, "%s", str);
            }
          return G_LOG_WRITER_HANDLED;
        }
    }
  else
    {
      return log_writer_default_custom (
        log_level, fields, n_fields, self);
      /*log_writer_standard_streams (*/
      /*log_level, fields, n_fields, self);*/
    }
}

/**
 * Initializes logging to a file.
 *
 * This must be called from the GTK thread.
 *
 * @param secs Number of timeout seconds.
 */
void
log_init_writer_idle (Log * self, unsigned int secs)
{
  self->writer_source_id = g_timeout_add_seconds (
    secs, (GSourceFunc) log_idle_cb, self);
}

static void *
create_log_event_obj (void)
{
  return object_new_unresizable (LogEvent);
}

static void
free_log_event_obj (LogEvent * ev)
{
  g_free_and_null (ev->message);

  object_zero_and_free_unresizable (LogEvent, ev);
}

#define LINE_SIZE 800
typedef struct Line
{
  char   line[LINE_SIZE]; // content
  size_t storage_sz;      // allocation size of line memory
  size_t
    sz; // size of line, not including terminating null byte ('\0')
} Line;

/**
 * Returns the last \ref n lines as a newly
 * allocated string.
 *
 * @note This must only be called from the GTK
 *   thread.
 *
 * @param n Number of lines.
 */
char *
log_get_last_n_lines (Log * self, int n)
{
  g_return_val_if_fail (self->log_filepath, NULL);

  /* call idle_cb to write queued messages */
  log_idle_cb (self);

  FILE * fp = fopen (self->log_filepath, "r");
  g_return_val_if_fail (fp, g_strdup (""));

  /* the following algorithm is from
   * https://stackoverflow.com/questions/54834969/print-last-few-lines-of-a-text-file
   * licensed under CC-BY-SA 4.0
   * authored by Bo R and edited
   * by Alexandros Theodotou */

  /* keep an extra slot for the last failed
   * read at EOF */
  Line * lines = g_malloc0_n ((size_t) n + 1, sizeof (Line));
  int    end = 0;
  int    size = 0;

  /* only keep track of the last couple of
   * lines */
  while (fgets (&lines[end].line[0], LINE_SIZE, fp))
    {
      lines[end].line[LINE_SIZE - 1] = '\0';
      lines[end].sz = strlen (lines[end].line);
      end++;
      if (end > n)
        {
          end = 0;
        }
      if (size < n)
        {
          size++;
        }
    }

  // time to print them back
  int first = end - size;
  if (first < 0)
    {
      first += size + 1;
    }
  GString * str = g_string_new (NULL);
  for (int count = size; count; count--)
    {
      g_string_append (str, lines[first].line);
      first++;
      if (first > size)
        {
          first = 0;
        }
    }

  /* clear up memory after use */
  free (lines);

  fclose (fp);

  return g_string_free (str, false);
}

/**
 * Initializes logging to a file.
 *
 * This can be called from any thread.
 *
 * @param filepath If non-NULL, the given file will be used,
 *   otherwise the default file will be created.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT bool
log_init_with_file (
  Log *        self,
  const char * filepath,
  GError **    error)
{
  /* open file to write to */
  if (filepath)
    {
      self->log_filepath = g_strdup (filepath);
      self->logfile = fopen (self->log_filepath, "a");
      if (!self->logfile)
        {
          g_set_error (
            error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
            "Failed to open logfile at %s",
            self->log_filepath);
          return false;
        }
    }
  else
    {
      char * str_datetime = datetime_get_for_filename ();
      char * user_log_dir =
        zrythm_get_dir (ZRYTHM_DIR_USER_LOG);
      self->log_filepath = g_strdup_printf (
        "%s%slog_%s.log", user_log_dir, G_DIR_SEPARATOR_S,
        str_datetime);
      GError * err = NULL;
      bool     success = io_mkdir (user_log_dir, &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "Failed to make log directory %s",
            user_log_dir);
          return false;
        }
      self->logfile = fopen (self->log_filepath, "a");
      if (!self->logfile)
        {
          g_set_error (
            error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
            "Failed to open logfile at %s",
            self->log_filepath);
          return false;
        }
      g_free (user_log_dir);
      g_free (str_datetime);
    }

  /* init buffers */
  /*self->messages_buf = gtk_text_buffer_new (NULL);*/

  /* init the object pool for log events */
  self->obj_pool = object_pool_new (
    create_log_event_obj, (ObjectFreeFunc) free_log_event_obj,
    MESSAGES_MAX);

  /* init the message queue */
  self->mqueue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->mqueue, (size_t) MESSAGES_MAX * sizeof (char *));

  self->initialized = true;

  return true;
}

static guint
g_parse_debug_envvar (
  const gchar *     envvar,
  const GDebugKey * keys,
  gint              n_keys,
  guint             default_value)
{
  const gchar * value;

#ifdef OS_WIN32
  /* "fatal-warnings,fatal-criticals,all,help" is pretty short */
  gchar buffer[100];

  if (GetEnvironmentVariable (envvar, buffer, 100) < 100)
    value = buffer;
  else
    return 0;
#else
  value = getenv (envvar);
#endif

  if (value == NULL)
    return default_value;

  return g_parse_debug_string (value, keys, (guint) n_keys);
}

/**
 * Generates a compressed log file (for sending with
 * bug reports).
 *
 * @return Whether successful.
 */
bool
log_generate_compressed_file (
  Log *     self,
  char **   ret_dir,
  char **   ret_path,
  GError ** error)
{
  g_return_val_if_fail (
    *ret_dir == NULL && *ret_path == NULL, false);

  GError * err = NULL;
  char *   log_file_tmpdir =
    g_dir_make_tmp ("zrythm-log-file-XXXXXX", &err);
  if (!log_file_tmpdir)
    {
      g_set_error_literal (
        error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
        "Failed to create temporary dir");
      return false;
    }

  /* get zstd-compressed text */
  char * log_txt = log_get_last_n_lines (LOG, 40000);
  g_return_val_if_fail (log_txt, false);
  size_t log_txt_sz = strlen (log_txt);
  size_t compress_bound = ZSTD_compressBound (log_txt_sz);
  char * dest = malloc (compress_bound);
  size_t dest_size = ZSTD_compress (
    dest, compress_bound, log_txt, log_txt_sz, 1);
  if (ZSTD_isError (dest_size))
    {
      free (dest);

      g_set_error (
        error, Z_UTILS_LOG_ERROR, Z_UTILS_LOG_ERROR_FAILED,
        "Failed to compress log text: %s",
        ZSTD_getErrorName (dest_size));

      g_free (log_file_tmpdir);
      return false;
    }

  /* write to dest file */
  char * dest_filepath =
    g_build_filename (log_file_tmpdir, "log.txt.zst", NULL);
  bool ret = g_file_set_contents (
    dest_filepath, dest, (gssize) dest_size, error);
  g_free (dest);
  if (!ret)
    {
      g_free (log_file_tmpdir);
      g_free (dest_filepath);
      return false;
    }

  *ret_dir = log_file_tmpdir;
  *ret_path = dest_filepath;

  return true;
}

/**
 * Returns a pointer to the global zlog.
 */
Log **
log_get (void)
{
  return &zlog;
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

  const GDebugKey keys[] = {
    {"gc-friendly",      1                     },
    { "fatal-warnings",
     G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL},
    { "fatal-criticals", G_LOG_LEVEL_CRITICAL  }
  };
  GLogLevelFlags flags = g_parse_debug_envvar (
    "G_DEBUG", keys, G_N_ELEMENTS (keys), 0);

  log_always_fatal |= flags & G_LOG_LEVEL_MASK;

  self->log_domains = g_strdup (g_getenv ("G_MESSAGES_DEBUG"));
  self->use_structured_for_console = true;
  self->min_log_level_for_test_console = G_LOG_LEVEL_MESSAGE;

  static bool log_writer_set = false;
  if (!log_writer_set)
    {
      log_writer_set = true;
      g_log_set_writer_func (
        (GLogWriterFunc) log_writer, self, NULL);
      use_default_log_writer = false;
    }

  /* remove temporary log file if it exists */
  if (tmp_log_file)
    {
      io_remove (tmp_log_file);
      g_free_and_null (tmp_log_file);
    }

  return self;
}

/**
 * Stops logging and frees any allocated memory.
 */
void
log_free (Log * self)
{
  g_message ("%s: Tearing down...", __func__);

  /* remove source func */
  if (self->writer_source_id)
    {
      g_source_remove (self->writer_source_id);
    }

  /* clear the queue */
  log_idle_cb (self);

  /* close log file */
  if (self->logfile)
    {
      fclose (self->logfile);
      self->logfile = NULL;
    }

  /* switch to default log writer */
  use_default_log_writer = true;

  /* free children */
  object_free_w_func_and_null (
    object_pool_free, self->obj_pool);
  object_free_w_func_and_null (mpmc_queue_free, self->mqueue);
  /*g_object_unref_and_null (self->messages_buf);*/

  g_free_and_null (self->log_domains);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
