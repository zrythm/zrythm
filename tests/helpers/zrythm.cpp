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

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL

#include "project/project_init_flow_manager.h"
#include "settings/g_settings_manager.h"
#include "utils/io.h"
#include "utils/pcg_rand.h"
#include "utils/rt_thread_id.h"

#include "tests/helpers/zrythm_helper.h"

#include "doctest_wrapper.h"
// #include "gtk_wrapper.h"

static void
z_g_test_watcher_remove_pid (GPid pid);

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-unsafe-call-within-signal-handler"
#endif
static void
segv_handler (int sig)
{
  std::string prefix;
#ifdef _WIN32
  prefix = _ ("Error - Backtrace:\n");
#else
  prefix = format_str (_ ("Error: {} - Backtrace:\n"), strsignal (sig));
#endif
  auto bt = Backtrace ().get_backtrace (prefix, 100, false);

  z_warning ("{}", bt);

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

      job = CreateJobObjectW (nullptr, nullptr);
      memset (&info, 0, sizeof (info));
      info.BasicLimitInformation.LimitFlags =
        0x2000 /* JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE */;

      if (!SetInformationJobObject (
            job, JobObjectExtendedLimitInformation, &info, sizeof (info)))
        z_warning (
          "Can't enable JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE: %s",
          g_win32_error_message (GetLastError ()));

      g_once_init_leave (&started, (gsize) job);
    }

  job = (HANDLE) started;

  if (!AssignProcessToJobObject (job, pid))
    z_warning (
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
      g_io_channel_read_line (channel, &command, nullptr, nullptr, &error);
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
              z_warning ("unknown pid {} to remove", pid);
            }
        }
      else
        {
          z_warning ("unknown command from parent '{}'", command);
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
          z_warning ("pipe() failed: {}", g_strerror (errsv));
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
          z_warning ("fork() failed: {}", g_strerror (errsv));
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
    status = g_io_channel_write_chars (channel, command, -1, nullptr, &error);
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
z_g_test_watcher_remove_pid (GPid pid)
{
  gchar * command;

  command = g_strdup_printf (REMOVE_PID_FORMAT, (guint) pid);
  watcher_send_command (command);
  g_free (command);
}

#endif

/* -------------------------------------------------------------------------- */
/* Utilities to cleanup the mess in the case unit test process crash */

static void
stop_daemon (Zrythm * self)
{
#ifdef _WIN32
  if (!TerminateProcess (self->pipewire_pid, 0))
    z_warning (
      "Can't terminate process: %s", g_win32_error_message (GetLastError ()));
#else
  kill (self->pipewire_pid_, SIGTERM);
#endif
  z_g_test_watcher_remove_pid (self->pipewire_pid_);
  g_spawn_close_pid (self->pipewire_pid_);
  self->pipewire_pid_ = 0;
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
    gtk_settings_get_default (), "gtk-theme-name", "Matcha-dark-sea", nullptr);
  z_info ("set theme");

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

  z_info ("set resource paths");

  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (
    css_provider, "/org/zrythm/Zrythm/app/theme.css");
  gtk_style_context_add_provider_for_display (
    display, GTK_STYLE_PROVIDER (css_provider), 800);
  g_object_unref (css_provider);
  z_info ("set default css provider");

  UI_CACHES = std::make_unique<UiCaches> ();
}

static void
start_daemon (Zrythm * self)
{
  const gchar * argv[] = { PIPEWIRE_BIN, "-c", PIPEWIRE_CONF_PATH, NULL };
  gint          pipe_fds[2] = { -1, -1 };
  GError *      error = NULL;

  make_pipe (pipe_fds, &error);
  g_assert_no_error (error);

  z_info ("print address: {}", pipe_fds[1]);

  auto pipewire_runtime_dir = Glib::getenv ("PIPEWIRE_RUNTIME_DIR");
  REQUIRE_NOTHROW (io_mkdir (pipewire_runtime_dir));

  /* Spawn pipewire */
  g_spawn_async_with_pipes_and_fds (
    nullptr, argv, nullptr,
    /* We Need this to get the pid returned on win32 */
    G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH |
      /* dbus-daemon will not abuse our descriptors, and
       * passing this means we can use posix_spawn() for speed */
      G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
    nullptr, nullptr, -1, -1, -1, &pipe_fds[1], &pipe_fds[1], 1,
    &self->pipewire_pid_, nullptr, nullptr, nullptr, &error);
  REQUIRE_NULL (error);

  _g_test_watcher_add_pid (self->pipewire_pid_);
}

ZrythmFixture::ZrythmFixture (
  bool optimized,
  int  samplerate,
  int  buf_size,
  bool use_pipewire,
  bool logging_enabled)
{
  static std::once_flag glib_inited;
  std::call_once (glib_inited, [] () { Glib::init (); });

  /* initialize logger */
  auto * logger = Logger::getInstance ();
  if (!logging_enabled)
    {
      logger->get_logger ()->set_level (spdlog::level::off);
    }

  /* dummy ZrythmApp object for testing */
  zrythm_app = zrythm_app_new (0, nullptr);
  // zrythm_app->gtk_thread_id = current_thread_id.get ();
  zrythm_app->samplerate = samplerate;
  zrythm_app->buf_size = buf_size;

  gZrythm->getInstance ()->pre_init (nullptr, false, optimized);
  REQUIRE_NONNULL (gZrythm);
  gZrythm->undo_stack_len_ = 64;
  char * version = Zrythm::get_version (false);
  z_info ("{}", version);
  g_free (version);
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  auto   zrythm_dir = dir_mgr->get_dir (ZrythmDirType::USER_TOP);
  REQUIRE_NONEMPTY (zrythm_dir);
  gZrythm->init ();
  z_info ("{}", zrythm_dir);

  if (use_pipewire)
    {
#ifndef HAVE_PIPEWIRE
      g_error ("pipewire program not found but requested pipewire engine");
#endif
      gZrythm->use_pipewire_in_tests_ = use_pipewire;
      start_daemon (gZrythm);
    }

  /* init logic - note: will use a random dir in tmp as the user dir */
  REQUIRE_NOTHROW (gZrythm->init_user_dirs_and_files ());
  gZrythm->init_templates ();

  {
    auto dir = g_dir_make_tmp ("zrythm_test_project_XXXXXX", nullptr);
    gZrythm->create_project_path_ = dir;
    g_free (dir);
  }
  zrythm_app->project_init_flow_mgr = std::make_unique<ProjectInitFlowManager> (
    "", false, test_helper_project_init_done_cb, nullptr);

  /* adaptive snap only supported with UI */
  SNAP_GRID_TIMELINE->snap_adaptive_ = false;

  /* set a segv handler */
  signal (SIGSEGV, segv_handler);

  z_info ("ZrythmFixture constructed");
}

ZrythmFixture::~ZrythmFixture ()
{
  z_info ("destroying ZrythmFixture");
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  dir_mgr->remove_testing_dir ();
  gZrythm->project_->audio_engine_->activate (false);
  gZrythm->project_.reset ();
  if (gZrythm->use_pipewire_in_tests_)
    {
      stop_daemon (gZrythm);
    }
  Zrythm::deleteInstance ();
  zrythm_app.reset ();

  ZrythmDirectoryManager::deleteInstance ();
  PCGRand::deleteInstance ();
  GSettingsManager::deleteInstance ();
  Logger::deleteInstance ();
}

void
test_helper_project_init_done_cb (
  bool        success,
  std::string error,
  void *      user_data)
{
  if (!success)
    {
      z_warning ("project init failed: {}", error);
    }
  REQUIRE (success);
}
