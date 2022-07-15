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
#  include <windows.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/system.h"

#include <gtk/gtk.h>

#include <reproc/drain.h>
#include <reproc/reproc.h>
#include <reproc/run.h>

/**
 * Runs the given command in the background, waits
 * for it to finish and returns its exit code.
 *
 * @param args NULL-terminated array of args.
 * @param[out] out_stdout A pointer to save the newly
 *   allocated stdout output (if non-NULL).
 * @param[out] out_stderr A pointer to save the newly
 *   allocated stderr output (if non-NULL).
 * @param ms_timer A timer in ms to
 *   kill the process, or negative to not
 *   wait.
 */
int
system_run_cmd_w_args (
  const char ** args,
  int           ms_to_wait,
  char **       out_stdout,
  char **       out_stderr,
  bool          warn_if_fail)
{
  g_message ("ms to wait: %d", ms_to_wait);

  reproc_options opts;
  memset (&opts, 0, sizeof (reproc_options));
  opts.stop.first.action = REPROC_STOP_WAIT;
  opts.stop.first.timeout = REPROC_DEADLINE;
  opts.stop.second.action = REPROC_STOP_TERMINATE;
  opts.stop.second.timeout = 1000;
  opts.stop.third.action = REPROC_STOP_KILL;
  opts.stop.third.timeout = REPROC_INFINITE;
  opts.deadline = ms_to_wait;

  reproc_sink
    stdout_sink = REPROC_SINK_NULL,
    stderr_sink = REPROC_SINK_NULL;
  if (out_stdout)
    {
      *out_stdout = NULL;
      stdout_sink = reproc_sink_string (out_stdout);
    }
  if (out_stderr)
    {
      *out_stderr = NULL;
      stderr_sink = reproc_sink_string (out_stderr);
    }
  int r = reproc_run_ex (args, opts, stdout_sink, stderr_sink);

  if (out_stdout)
    {
      g_message ("stdout: %s", *out_stdout);
    }
  if (out_stderr)
    {
      g_message ("stderr: %s", *out_stderr);
    }

  if (r < 0)
    {
      if (warn_if_fail)
        {
          g_warning ("%s", reproc_strerror (r));
        }
      else
        {
          g_message ("%s", reproc_strerror (r));
        }
    }

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
system_run_cmd (const char * cmd, long ms_timer)
{
#ifdef _WOE32
  STARTUPINFO         si;
  PROCESS_INFORMATION pi;
  ZeroMemory (&si, sizeof (si));
  si.cb = sizeof (si);
  ZeroMemory (&pi, sizeof (pi));
  g_message ("attempting to run process %s", cmd);
  BOOL b = CreateProcess (
    NULL, cmd, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL,
    &si, &pi);
  if (!b)
    {
      g_critical ("create process failed for %s", cmd);
      return -1;
    }
  /* wait for process to end */
  DWORD dwMilliseconds =
    ms_timer >= 0 ? (DWORD) ms_timer : INFINITE;
  WaitForSingleObject (pi.hProcess, dwMilliseconds);
  DWORD dwExitCode = 0;
  GetExitCodeProcess (pi.hProcess, &dwExitCode);
  /* close process and thread handles */
  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);
  g_message (
    "windows process exit code: %d", (int) dwExitCode);
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
  GIOChannel *     channel,
  GIOCondition     cond,
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
  GPid     pid;
  int      out, err;
  GError * g_err = NULL;
  bool     ret = g_spawn_async_with_pipes (
        NULL, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, &pid, NULL,
        &out, &err, &g_err);
  if (!ret)
    {
      g_warning (
        "(%s) spawn failed: %s", __func__, g_err->message);
      return NULL;
    }

  /* create channels used to read output */
  GIOChannel * out_ch =
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
        out_ch, G_IO_IN, (GIOFunc) watch_out_cb, &data);

      gint64 time_at_start = g_get_monotonic_time ();
      gint64 cur_time = time_at_start;
      while (
        !data.exited
        && (cur_time - time_at_start) < (1000 * ms_timer))
        {
          g_usleep (10000);
          cur_time = g_get_monotonic_time ();
        }
      g_usleep (10000);
    }

  g_spawn_close_pid (pid);

  char *    str;
  gsize     size;
  GIOStatus gio_status =
    g_io_channel_read_to_end (out_ch, &str, &size, NULL);
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
