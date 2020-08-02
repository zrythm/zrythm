/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef _WOE32
#include <windows.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/system.h"

#include <gtk/gtk.h>

#include <reproc/reproc.h>

/**
 * Runs the given command in the background, waits
 * for it to finish and returns its exit code.
 *
 * @note Only works for stdout for now.
 *
 * @param args NULL-terminated array of args.
 * @param get_stdout Whether to get the standard out
 *   (true) or stderr (false).
 * @param[out] output A pointer to save the newly
 *   allocated stdout or stderr output.
 * @param ms_timer A timer in ms to
 *   kill the process, or negative to not
 *   wait.
 */
int
system_run_cmd_w_args (
  const char ** args,
  int           ms_to_wait,
  bool          get_stdout,
  char **       output,
  bool          warn_if_fail)
{
  g_message ("ms to wait: %d", ms_to_wait);

  *output = NULL;
  size_t size = 0;
  int r = REPROC_ENOMEM;
  reproc_event_source children[1];
  bool have_events = false;
  bool read_once = false;

  reproc_options opts;
  memset (&opts, 0, sizeof (reproc_options));
  opts.stop.first.action = REPROC_STOP_WAIT;
  opts.stop.first.timeout = 100;
  opts.stop.second.action = REPROC_STOP_TERMINATE;
  opts.stop.second.timeout = 100;
  opts.stop.third.action = REPROC_STOP_KILL;
  opts.stop.third.timeout = 100;
  opts.deadline = ms_to_wait;
  opts.nonblocking = true;

#if 0
  reproc_stop_actions stop;
  stop.first.action = REPROC_STOP_WAIT;
  stop.first.timeout = 100;
  stop.second.action = REPROC_STOP_TERMINATE;
  stop.second.timeout = 100;
  stop.third.action = REPROC_STOP_KILL;
  stop.third.timeout = 10;
#endif

  reproc_t * process = reproc_new ();

  char err_str[8000];

  if (!process)
    {
      sprintf (
        err_str,
        "create process failed for %s", args[0]);
      goto finish;
    }

  g_message ("starting...");
  r = reproc_start (process, args, opts);
  if (r < 0)
    {
      sprintf (
        err_str,
        "process failed to start for %s",
        args[0]);
      goto finish;
    }

  g_message ("closing...");
  r = reproc_close (process, REPROC_STREAM_IN);
  if (r < 0)
    {
      sprintf (
        err_str,
        "process failed to close for %s",
        args[0]);
      goto finish;
    }

  children[0].process = process;
  children[0].interests =
    get_stdout ? REPROC_EVENT_OUT : REPROC_EVENT_ERR;

  for (;;)
    {
      if (r < 0)
        {
          r = r == REPROC_EPIPE ? 0 : r;
          goto finish;
        }

      uint8_t buffer[4096];
      g_message ("polling for %d ms...", ms_to_wait);
      reproc_poll (children, 1, ms_to_wait);
      g_message ("polled");
      if (children[0].events)
        {
          have_events = true;
        }
      else
        {
          g_message ("no events");
          break;
        }

      g_message ("reading...");
      r =
        reproc_read (
          process,
          get_stdout ?
            REPROC_STREAM_OUT : REPROC_STREAM_ERR,
          buffer, sizeof (buffer));
      if (r < 0)
        {
          g_message ("failed during read");
          if (read_once)
            {
              r = 0;
            }
          break;
        }
      g_message ("read");
      read_once = true;

      size_t bytes_read = (size_t) r;

      char * result =
        realloc (*output, size + bytes_read + 1);
      if (result == NULL)
        {
          r = REPROC_ENOMEM;
          g_message ("ENOMEM");
          goto finish;
        }

      *output = result;

      // Copy new data into `output`.
      memcpy(*output + size, buffer, bytes_read);
      (*output)[size + bytes_read] = '\0';
      size += bytes_read;

      if (r == REPROC_EPIPE)
        {
          break;
        }
    }

  if (have_events && !read_once && r < 0)
    {
      sprintf (
        err_str,
        "failed to get output from process %s",
        args[0]);
      goto finish;
    }

  if (*output)
    {
      g_message ("output:\n%s", *output);
    }
  else
    {
      g_message ("no output");
    }

  g_message ("waiting...");
  /* uses deadline */
  r = reproc_wait (process, REPROC_DEADLINE);
  g_message ("waited");

  if (r < 0)
    {
      sprintf (
        err_str,
        "failed to wait for process %s",
        args[0]);
      goto finish;
    }

  finish:
  g_message ("finishing...");

  if (r < 0)
    {
      if (warn_if_fail)
        {
          g_warning ("%s", err_str);
          g_warning ("%s", reproc_strerror (r));
        }
      else
        {
          g_message ("%s", err_str);
          g_message ("%s", reproc_strerror (r));
        }
    }

  g_message ("destroying...");
  reproc_destroy (process);
  g_message ("destroyed");

  return (r < 0) ? r : 0;
}


/**
 * Runs the given command in the background, waits for
 * it to finish and returns its exit code.
 *
 * @param ms_timer A timer in ms to
 *   kill the process, or negative to not
 *   wait.
 */
int
system_run_cmd (
  const char * cmd,
  long         ms_timer)
{
#ifdef _WOE32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );
  g_message ("attempting to run process %s",  cmd);
  BOOL b =
    CreateProcess (
      NULL, cmd, NULL, NULL, TRUE,
      DETACHED_PROCESS, NULL, NULL, &si, &pi);
  if (!b)
    {
      g_critical ("create process failed for %s", cmd);
      return -1;
    }
  /* wait for process to end */
  DWORD dwMilliseconds =
    ms_timer >= 0 ?
    (DWORD) ms_timer : INFINITE;
  WaitForSingleObject (
    pi.hProcess, dwMilliseconds);
  DWORD dwExitCode = 0;
  GetExitCodeProcess (pi.hProcess, &dwExitCode);
  /* close process and thread handles */
  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);
  g_message (
    "windows process exit code: %d",
    (int) dwExitCode);
  return (int) dwExitCode;
#else
  char timed_cmd[8000];
  if (ms_timer >= 0)
    {
      sprintf (
        timed_cmd, "timeout %ld bash -c \"%s\"",
        ms_timer / 1000, cmd);
    }
  return system (timed_cmd);
#endif
}

typedef struct ChildWatchData
{
  bool exited;
} ChildWatchData;

static gboolean
watch_out_cb (
  GIOChannel   *channel,
  GIOCondition  cond,
  ChildWatchData * data)
{
  data->exited = true;

  return true;
}

/**
 * Runs the command and returns the output, or NULL.
 *
 * This assumes that the process will exit within a
 * few milliseconds from when the first output is
 * printed, unless \ref always_wait is true, in
 * which case the process will only be
 * reaped after the waiting time.
 *
 * @param ms_timer A timer in ms to
 *   kill the process, or negative to not
 *   wait.
 */
char *
system_get_cmd_output (
  char ** argv,
  long    ms_timer,
  bool    always_wait)
{
  GPid pid;
  int out, err;
  GError * g_err = NULL;
  bool ret =
    g_spawn_async_with_pipes (
      NULL, argv, NULL, G_SPAWN_DEFAULT, NULL,
      NULL, &pid, NULL, &out, &err, &g_err);
  if (!ret)
    {
      g_warning (
        "(%s) spawn failed: %s",
        __func__, g_err->message);
      return NULL;
    }

  /* create channels used to read output */
  GIOChannel *out_ch =
#ifdef _WOE32
    g_io_channel_win32_new_fd (out);
#else
    g_io_channel_unix_new (out);
#endif

  if (always_wait)
    {
      /* wait for the full length of the timer */
      g_usleep (1000 * (unsigned long) ms_timer);
    }
  else
    {
      /* wait until the first input is received or
       * for the full length of the timer if no
       * input is received */
      ChildWatchData data = { false };
      g_io_add_watch (
        out_ch, G_IO_IN, (GIOFunc) watch_out_cb,
        &data);

      gint64 time_at_start = g_get_monotonic_time ();
      gint64 cur_time = time_at_start;
      while (!data.exited &&
             (cur_time - time_at_start) <
               (1000 * ms_timer))
        {
          g_usleep (10000);
          cur_time = g_get_monotonic_time ();
        }
      g_usleep (10000);
    }

  g_spawn_close_pid (pid);

  char * str;
  gsize size;
  GIOStatus gio_status =
    g_io_channel_read_to_end (
      out_ch, &str, &size, NULL);
  g_io_channel_unref (out_ch);

  if (gio_status == G_IO_STATUS_NORMAL)
    return str;
  else
    return NULL;

#if 0
  /* Open the command for reading. */
  FILE * fp = popen (cmd, "r");
  g_return_val_if_fail (fp, NULL);

  /* Read the output a line at a time - output it. */
  const int size = 4000;
  char buf[size];
  GString * str = g_string_new (NULL);
  while (fgets (buf, size, fp) != NULL)
    {
      g_string_append (str, buf);
    }

  /* close */
  pclose (fp);

  return g_string_free (str, false);
#endif
}
