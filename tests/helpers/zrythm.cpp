// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 *  * Copyright (C) 2008-2010 Red Hat, Inc.
 * Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors: David Zeuthen <davidz@redhat.com>
 *          Xavier Claessens <xavier.claessens@collabora.co.uk>o
 *
 * ---
 */

#include "settings/g_settings_manager.h"
#include "utils/pcg_rand.h"

#include "tests/helpers/zrythm_helper.h"

#include "gtk_wrapper.h"

static void
_g_test_watcher_remove_pid (GPid pid);

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-unsafe-call-within-signal-handler"
#endif
static void
segv_handler (int sig)
{
  char prefix[200];
#ifdef _WIN32
  strcpy (prefix, _ ("Error - Backtrace:\n"));
#else
  sprintf (prefix, _ ("Error: %s - Backtrace:\n"), strsignal (sig));
#endif
  char * bt = backtrace_get (prefix, 100, false);

  g_warning ("%s", bt);

  exit (sig);
}
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

static gboolean
make_pipe (gint pipe_fds[2], GError ** error)
{
#if defined(G_OS_UNIX)
  return g_unix_open_pipe (pipe_fds, FD_CLOEXEC, error);
#elif defined(_WIN32)
  if (_pipe (pipe_fds, 4096, _O_BINARY) < 0)
    {
      int errsv = errno;

      g_set_error (
        error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
        _ ("Failed to create pipe for communicating with child process (%s)"),
        g_strerror (errsv));
      return FALSE;
    }
  return TRUE;
#else
  g_set_error (
    error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
    _ ("Pipes are not supported in this platform"));
  return FALSE;
#endif
}

#ifdef _WIN32

/* This could be interesting to expose in public API */
static void
_g_test_watcher_add_pid (GPid pid)
{
  static gsize started = 0;
  HANDLE       job;

  if (g_once_init_enter (&started))
    {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;

      job = CreateJobObjectW (NULL, NULL);
      memset (&info, 0, sizeof (info));
      info.BasicLimitInformation.LimitFlags =
        0x2000 /* JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE */;

      if (!SetInformationJobObject (
            job, JobObjectExtendedLimitInformation, &info, sizeof (info)))
        g_warning (
          "Can't enable JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE: %s",
          g_win32_error_message (GetLastError ()));

      g_once_init_leave (&started, (gsize) job);
    }

  job = (HANDLE) started;

  if (!AssignProcessToJobObject (job, pid))
    g_warning (
      "Can't assign process to job: %s",
      g_win32_error_message (GetLastError ()));
}

static void
_g_test_watcher_remove_pid (GPid pid)
{
  /* No need to unassign the process from the job object as the process
     will be killed anyway */
}

#else

#  define ADD_PID_FORMAT "add pid %d\n"
#  define REMOVE_PID_FORMAT "remove pid %d\n"

static void
watch_parent (gint fd)
{
  GIOChannel * channel;
  GPollFD      fds[1];
  GArray *     pids_to_kill;

  channel = g_io_channel_unix_new (fd);

  fds[0].fd = fd;
  fds[0].events = G_IO_HUP | G_IO_IN;
  fds[0].revents = 0;

  pids_to_kill = g_array_new (FALSE, FALSE, sizeof (guint));

  do
    {
      gint     num_events;
      gchar *  command = NULL;
      guint    pid;
      guint    n;
      GError * error = NULL;

      num_events = g_poll (fds, 1, -1);
      if (num_events == 0)
        continue;

      if (fds[0].revents & G_IO_HUP)
        {
          /* Parent quit, cleanup the mess and exit */
          for (n = 0; n < pids_to_kill->len; n++)
            {
              pid = g_array_index (pids_to_kill, guint, n);
              g_printerr ("cleaning up pid %d\n", pid);
              kill ((__pid_t) pid, SIGTERM);
            }

          g_array_unref (pids_to_kill);
          g_io_channel_shutdown (channel, FALSE, &error);
          g_assert_no_error (error);
          g_io_channel_unref (channel);

          exit (0);
        }

      /* Read the command from the input */
      g_io_channel_read_line (channel, &command, NULL, NULL, &error);
      g_assert_no_error (error);

      /* Check for known commands */
      if (sscanf (command, ADD_PID_FORMAT, &pid) == 1)
        {
          g_array_append_val (pids_to_kill, pid);
        }
      else if (sscanf (command, REMOVE_PID_FORMAT, &pid) == 1)
        {
          for (n = 0; n < pids_to_kill->len; n++)
            {
              if (g_array_index (pids_to_kill, guint, n) == pid)
                {
                  g_array_remove_index (pids_to_kill, n);
                  pid = 0;
                  break;
                }
            }
          if (pid != 0)
            {
              g_warning ("unknown pid %d to remove", pid);
            }
        }
      else
        {
          g_warning ("unknown command from parent '%s'", command);
        }

      g_free (command);
    }
  while (TRUE);
}

#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
static GIOChannel *
watcher_init (void)
{
  static gsize        started = 0;
  static GIOChannel * channel = NULL;
  int                 errsv;

  if (g_once_init_enter (&started))
    {
      gint pipe_fds[2];

      /* fork a child to clean up when we are killed */
      if (pipe (pipe_fds) != 0)
        {
          errsv = errno;
          g_warning ("pipe() failed: %s", g_strerror (errsv));
          g_assert_not_reached ();
        }

      /* flush streams to avoid buffers being duplicated in the child and
       * flushed by both the child and parent later
       *
       * FIXME: This is a workaround for the fact that watch_parent() uses
       * non-async-signal-safe API. See
       * https://gitlab.gnome.org/GNOME/glib/-/issues/2322#note_1034330
       */
      fflush (stdout);
      fflush (stderr);

      switch (fork ())
        {
        case -1:
          errsv = errno;
          g_warning ("fork() failed: %s", g_strerror (errsv));
          g_assert_not_reached ();
          break;

        case 0:
          /* child */
          close (pipe_fds[1]);
          watch_parent (pipe_fds[0]);
          break;

        default:
          /* parent */
          close (pipe_fds[0]);
          channel = g_io_channel_unix_new (pipe_fds[1]);
        }

      g_once_init_leave (&started, 1);
    }

  return channel;
}
#  pragma GCC diagnostic pop

static void
watcher_send_command (const gchar * command)
{
  GIOChannel * channel;
  GError *     error = NULL;
  GIOStatus    status;

  channel = watcher_init ();

  do
    status = g_io_channel_write_chars (channel, command, -1, NULL, &error);
  while (status == G_IO_STATUS_AGAIN);
  g_assert_no_error (error);

  g_io_channel_flush (channel, &error);
  g_assert_no_error (error);
}

/* This could be interesting to expose in public API */
static void
_g_test_watcher_add_pid (GPid pid)
{
  gchar * command;

  command = g_strdup_printf (ADD_PID_FORMAT, (guint) pid);
  watcher_send_command (command);
  g_free (command);
}

static void
_g_test_watcher_remove_pid (GPid pid)
{
  gchar * command;

  command = g_strdup_printf (REMOVE_PID_FORMAT, (guint) pid);
  watcher_send_command (command);
  g_free (command);
}

#endif

/* -------------------------------------------------------------------------- */
/* Utilities to cleanup the mess in the case unit test process crash */

typedef struct ZrythmTestPipewire
{
} ZrythmTestPipewire;

static void
stop_daemon (Zrythm * self)
{
#ifdef _WIN32
  if (!TerminateProcess (self->pipewire_pid, 0))
    g_warning (
      "Can't terminate process: %s", g_win32_error_message (GetLastError ()));
#else
  kill (self->pipewire_pid, SIGTERM);
#endif
  _g_test_watcher_remove_pid (self->pipewire_pid);
  g_spawn_close_pid (self->pipewire_pid);
  self->pipewire_pid = 0;
}

/**
 * To be called after test_helper_zrythm_init() to initialize the UI (GTK).
 */
void
test_helper_zrythm_gui_init (int argc, char * argv[])
{
  gZrythm->have_ui_ = true;
  if (gtk_init_check ())
    {
      gtk_init ();
    }
  else
    {
      g_test_skip ("No display found");
      exit (77);
    }

  /* set theme */
  g_object_set (
    gtk_settings_get_default (), "gtk-theme-name", "Matcha-dark-sea", NULL);
  g_message ("set theme");

  GdkDisplay *   display = gdk_display_get_default ();
  GtkIconTheme * icon_theme = gtk_icon_theme_get_for_display (display);
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/gnome-builder");

  g_message ("set resource paths");

  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (
    css_provider, "/org/zrythm/Zrythm/app/theme.css");
  gtk_style_context_add_provider_for_display (
    display, GTK_STYLE_PROVIDER (css_provider), 800);
  g_object_unref (css_provider);
  g_message ("set default css provider");

  UI_CACHES = ui_caches_new ();
}

static void
start_daemon (Zrythm * self)
{
  const gchar * argv[] = { PIPEWIRE_BIN, "-c", PIPEWIRE_CONF_PATH, NULL };
  gint          pipe_fds[2] = { -1, -1 };
  GError *      error = NULL;

  make_pipe (pipe_fds, &error);
  g_assert_no_error (error);

  g_message ("print address: %d", pipe_fds[1]);

  const char * pipewire_runtime_dir = g_getenv ("PIPEWIRE_RUNTIME_DIR");
  bool         success = io_mkdir (pipewire_runtime_dir, &error);
  g_assert_true (success);
  g_assert_no_error (error);

  /* Spawn pipewire */
  g_spawn_async_with_pipes_and_fds (
    NULL, argv, NULL,
    /* We Need this to get the pid returned on win32 */
    G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH |
      /* dbus-daemon will not abuse our descriptors, and
       * passing this means we can use posix_spawn() for speed */
      G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
    NULL, NULL, -1, -1, -1, &pipe_fds[1], &pipe_fds[1], 1, &self->pipewire_pid,
    NULL, NULL, NULL, &error);
  g_assert_no_error (error);

  _g_test_watcher_add_pid (self->pipewire_pid);
}

void
_test_helper_zrythm_init (
  bool optimized,
  int  samplerate,
  int  buf_size,
  bool use_pipewire)
{
  if (gZrythm)
    {
      object_free_w_func_and_null (project_free, PROJECT);
    }
  gZrythm.reset ();
  object_free_w_func_and_null (log_free, LOG);

  /* dummy ZrythmApp object for testing */
  zrythm_app = object_new (ZrythmApp);
  zrythm_app->gtk_thread = g_thread_self ();
  zrythm_app->samplerate = samplerate;
  zrythm_app->buf_size = buf_size;

  Log ** log_ptr = log_get ();
  Log *  log_obj = log_new ();
  *log_ptr = log_obj;
  LOG = log_obj;

  gZrythm.reset (new Zrythm (nullptr, false, optimized));
  g_assert_true (gZrythm != nullptr);
  gZrythm->undo_stack_len = 64;
  char * version = Zrythm::get_version (false);
  g_message ("%s", version);
  g_free (version);
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  char * zrythm_dir = dir_mgr->get_dir (USER_TOP);
  g_assert_nonnull (zrythm_dir);
  g_message ("%s", zrythm_dir);
  gZrythm->init ();

  if (use_pipewire)
    {
#ifndef HAVE_PIPEWIRE
      g_error ("pipewire program not found but requested pipewire engine");
#endif
      gZrythm->use_pipewire_in_tests = use_pipewire;
      start_daemon (gZrythm.get ());
    }

  /* init logic - note: will use a random dir in
   * tmp as the user dir */
  GError * err = NULL;
  bool     success = gZrythm->init_user_dirs_and_files (&err);
  g_assert_true (success);
  gZrythm->init_templates ();

  /* init logging to custom file */
  char * tmp_log_dir =
    g_build_filename (g_get_tmp_dir (), "zrythm_test_logs", NULL);
  success = io_mkdir (tmp_log_dir, NULL);
  g_assert_true (success);
  char * str_datetime = datetime_get_for_filename ();
  char * log_filepath = g_strdup_printf (
    "%s%slog_%s.log", tmp_log_dir, G_DIR_SEPARATOR_S, str_datetime);
  g_free (str_datetime);
  g_free (tmp_log_dir);
  success = log_init_with_file (LOG, log_filepath, NULL);
  g_assert_true (success);
  log_init_writer_idle (LOG, 1);

  gZrythm->create_project_path =
    g_dir_make_tmp ("zrythm_test_project_XXXXXX", NULL);
  project_init_flow_manager_load_or_create_default_project (
    NULL, false, test_helper_project_init_done_cb, NULL);

  /* adaptive snap only supported with UI */
  SNAP_GRID_TIMELINE->snap_adaptive = false;

  /* set a segv handler */
  signal (SIGSEGV, segv_handler);
}

/**
 * To be called by every test's main at the end to
 * clean up.
 */
void
test_helper_zrythm_cleanup (void)
{
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  dir_mgr->remove_testing_dir ();
  object_free_w_func_and_null (project_free, gZrythm->project);
  if (gZrythm->use_pipewire_in_tests)
    {
      stop_daemon (gZrythm.get ());
    }
  gZrythm.reset ();
  object_free_w_func_and_null (log_free, LOG);
  object_zero_and_free (zrythm_app);

  ZrythmDirectoryManager::deleteInstance ();
  PCGRand::deleteInstance ();
  GSettingsManager::deleteInstance ();
}

/**
 * To be called by every test's main to initialize
 * Zrythm to default values.
 */
void
test_helper_zrythm_init (void)
{
  _test_helper_zrythm_init (false, 0, 0, false);
}

void
test_helper_zrythm_init_with_pipewire (void)
{
  _test_helper_zrythm_init (false, 0, 0, true);
}

void
test_helper_zrythm_init_optimized (void)
{
  _test_helper_zrythm_init (true, 0, 0, false);
}

void
test_helper_project_init_done_cb (bool success, GError * error, void * user_data)
{
  g_assert_true (success);
}
