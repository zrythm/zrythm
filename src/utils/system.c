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

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "utils/system.h"

#include <gtk/gtk.h>

#if 0 // not tested, for reference only
/**
 * Fork and wait for child process to
 * finish or kill it.
 *
 * @return The pid of the child or 0 if
 *   parent.
 */
int
system_fork_with_timer (
  int   sec,
  int * exit_code)
{
#ifdef _WIN32
#else /* FIXME */
  pid_t pid = fork ();
  if (pid != 0) // parent
  {
    g_message ("waiting for vst plugin to end...");
    gint64 start_time =
      g_get_monotonic_time ();
    gint64 cur_time = start_time;
    while (cur_time < start_time + 10000000)
    {
    int stat;
    pid_t res =
      waitpid (pid, &stat, WNOHANG);
    if (pid == res)
    {
      /* exited normally */
      g_message ("plugin exited normally");
      return 0;
    }
    else if (res == 0)
    {
      /* still running */
      g_message ("still running");
    }
    else if (res == -1)
    {
      /* error */
      g_message ("error");
      kill (pid, SIGKILL);
      return -1;
    }
    g_usleep (1000);
    cur_time = g_get_monotonic_time ();
    }

    /* the child process is still
     * running - kill it and return -1 */
    kill (pid, SIGKILL);
    return -1;
  }
#endif
}
#endif

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
#ifdef _WIN32
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
