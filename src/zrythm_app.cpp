// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *  ---
 *
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2006-2008 Doug McLain <doug@nostar.net>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2006-2017 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2006 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013-2015 John Emmas <john@creativepost.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  ---
 */

#include "zrythm-config.h"

#include "utils/pcg_rand.h"

#ifdef _WIN32
#  include <stdio.h> // for _setmaxstdio
#else
#  include <sys/mman.h>
#  include <sys/resource.h>
#endif
#include <cstdlib>

#include "actions/actions.h"
#include "gui/backend/file_manager.h"
#include "gui/widgets/dialogs/ask_to_check_for_updates_dialog.h"
#include "gui/widgets/dialogs/bug_report_dialog.h"
#include "gui/widgets/greeter.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "project/project_init_flow_manager.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "settings/user_shortcuts.h"
#include "utils/backtrace.h"
#include "utils/dialogs.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/logger.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "utils/vamp.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "Wrapper.h"
#include "ext/whereami/whereami.h"
#include "gtk_wrapper.h"
#include <fftw3.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtksourceview/gtksource.h>
#ifdef HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif
/*#include <suil/suil.h>*/
#ifdef HAVE_X11
#  include <X11/Xlib.h>
#endif
#include <libpanel.h>

#include "libadwaita_wrapper.h"
#include <curl/curl.h>
#ifdef HAVE_VALGRIND
#  include <valgrind/valgrind.h>
#endif

G_DEFINE_TYPE (ZrythmApp, zrythm_app, GTK_TYPE_APPLICATION);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-unsafe-call-within-signal-handler"
/** SIGSEGV/SIGABRT handler. */
static void
segv_handler (int sig)
{
  /* stop this handler from being called multiple times */
  signal (SIGSEGV, SIG_DFL);

  char prefix[200];
#ifdef _WIN32
  strcpy (prefix, _ ("Error - Backtrace:\n"));
#else
  sprintf (prefix, _ ("Error: %s - Backtrace:\n"), strsignal (sig));
#endif

  /* avoid getting backtraces too often if a bug report dialog
   * is already opened - it makes it hard to type due to lag */
  std::string bt;
  if (!zrythm_app->bug_report_dialog)
    {
      bt = Backtrace ().get_backtrace (prefix, 100, true);
    }

  /* call the callback to write queued messages and get last few lines of the
   * log, before logging the backtrace */
  // log_idle_cb (LOG);
  z_info ("{}", bt);
  // log_idle_cb (LOG);

  if (MAIN_WINDOW)
    {
      if (!zrythm_app->bug_report_dialog)
        {
          char str[500];
          sprintf (str, _ ("%s has crashed. "), PROGRAM_NAME);
          GtkWindow * win = gtk_application_get_active_window (
            GTK_APPLICATION (zrythm_app.get ()));
          zrythm_app->bug_report_dialog = bug_report_dialog_new (
            win ? GTK_WIDGET (win) : nullptr, str, bt.c_str (), true);

          adw_dialog_present (
            ADW_DIALOG (zrythm_app->bug_report_dialog),
            win ? GTK_WIDGET (win) : nullptr);
        }

      return;
    }

  exit (EXIT_FAILURE);
}

/** SIGTERM handler. */
static void
sigterm_handler (int sig)
{
  if (zrythm_app)
    {
      g_application_quit (G_APPLICATION (zrythm_app.get ()));
    }
}

#pragma GCC diagnostic pop

ZrythmAppUiMessage *
zrythm_app_ui_message_new (GtkMessageType type, const char * msg)
{
  ZrythmAppUiMessage * self = object_new (ZrythmAppUiMessage);

  self->msg = g_strdup (msg);
  self->type = type;

  return self;
}

void
zrythm_app_ui_message_free (ZrythmAppUiMessage * self)
{
  g_free_and_null (self->msg);

  object_zero_and_free (self);
}

static void
check_for_updates_latest_release_ver_ready (
  GObject *      source_object,
  GAsyncResult * res,
  gpointer       data)
{
  GError * err = NULL;
  char *   latest_release = Zrythm::fetch_latest_release_ver_finish (res, &err);
  if (!latest_release)
    {
      HANDLE_ERROR_LITERAL (
        err, _ ("Failed fetching the latest release version"));
      return;
    }

  bool is_latest_release = Zrythm::is_latest_release (latest_release);
#ifdef HAVE_CHANGELOG
  /* if latest release and first run on this release show
   * CHANGELOG */
  if (
    is_latest_release
    && !GSettingsManager::strv_contains_str (
      S_GENERAL, "run-versions", PACKAGE_VERSION))
    {
      AdwMessageDialog * dialog = dialogs_get_basic_ok_message_dialog (nullptr);
      adw_message_dialog_format_heading (dialog, "%s", _ ("Changelog"));
      adw_message_dialog_format_body_markup (
        dialog, _ ("Running %s version <b>%s</b>%s%s"), PROGRAM_NAME,
        PACKAGE_VERSION, "\n\n", CHANGELOG_TXT);
      gtk_window_present (GTK_WINDOW (dialog));
      GSettingsManager::append_to_strv (
        S_GENERAL, "run-versions", PACKAGE_VERSION, true);
    }
#endif /* HAVE_CHANGELOG */

  /* if not latest release and this is an official release,
   * notify user */
  char * last_version_notified_on =
    g_settings_get_string (S_GENERAL, "last-version-new-release-notified-on");
  z_debug (
    "last version notified on: {}"
    "\n package version: {}",
    last_version_notified_on, PACKAGE_VERSION);
  if (
    !is_latest_release && Zrythm::is_release (true)
    && !string_is_equal (last_version_notified_on, PACKAGE_VERSION))
    {
      ui_show_message_printf (
        _ ("New Zrythm Version"),
        _ ("A new version of Zrythm has been released: "
           "<b>%s</b>\n\n"
           "Your current version is %s"),
        latest_release, PACKAGE_VERSION);

      g_settings_set_string (
        S_GENERAL, "last-version-new-release-notified-on", PACKAGE_VERSION);
    }
  g_free (last_version_notified_on);
}

/**
 * Handles the logic for checking for updates on startup.
 */
void
zrythm_app_check_for_updates (ZrythmApp * self)
{
  if (g_settings_get_boolean (S_GENERAL, "first-check-for-updates"))
    {
      /* this will call zrythm_app_check_for_updates() again later if the
       * response is yes */
      ask_to_check_for_updates_dialog_run_async (GTK_WIDGET (MAIN_WINDOW));
      return;
    }

  if (!g_settings_get_boolean (S_P_GENERAL_UPDATES, "check-for-updates"))
    return;

  Zrythm::fetch_latest_release_ver_async (
    check_for_updates_latest_release_ver_ready, nullptr);
}

/**
 * Initializes the array of recent projects in
 * Zrythm app.
 */
static void
init_recent_projects (void)
{
  z_info ("Initializing recent projects...");

  gchar ** recent_projects = g_settings_get_strv (S_GENERAL, "recent-projects");

  gZrythm->recent_projects_ =
    StringArray ((const char * const *) recent_projects);
  gZrythm->recent_projects_.removeDuplicates (false);

  /* save the new list */
  char ** tmp = gZrythm->recent_projects_.getNullTerminated ();
  g_settings_set_strv (S_GENERAL, "recent-projects", (const char * const *) tmp);
  g_strfreev (tmp);

  z_info ("done");
}

/**
 * Called after the main window and the project have been
 * initialized. Sets up the window using the backend.
 *
 * This is the final step.
 */
static void
on_setup_main_window (GSimpleAction * action, GVariant * parameter, gpointer data)
{
  z_info ("setting up...");

  ZrythmApp * self = ZRYTHM_APP (data);

  /* add timeout for auto-saving projects */
  unsigned int autosave_interval =
    g_settings_get_uint (S_P_PROJECTS_GENERAL, "autosave-interval");
  if (autosave_interval > 0)
    {
      PROJECT->last_successful_autosave_time_ = SteadyClock::now ();
      self->project_autosave_source_id =
        g_timeout_add_seconds (3, Project::autosave_cb, nullptr);
    }

  z_info ("done");
}

static void
no_project_selected_exit_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           data)
{
  exit (EXIT_SUCCESS);
}

static void
project_load_or_create_ready_cb (bool success, std::string msg, void * user_data)
{
  g_application_release (g_application_get_default ());
  if (!success)
    {
      /* FIXME never called */
      z_info ("no project selected. exiting...");
      AdwMessageDialog * dialog = dialogs_get_basic_ok_message_dialog (nullptr);
      adw_message_dialog_format_heading (
        dialog, "%s", _ ("No Project Selected"));
      adw_message_dialog_format_body_markup (
        dialog,
        _ ("No project has been selected or project failed to load. "
           "%s will now close."),
        PROGRAM_NAME);
      g_signal_connect (
        G_OBJECT (dialog), "response",
        G_CALLBACK (no_project_selected_exit_response_cb), nullptr);
      return;
    }

  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app.get ()), "setup_main_window", nullptr);
}

/**
 * Called after the main window has been initialized.
 *
 * Loads the project backend or creates the default one.
 */
static void
on_load_project (GSimpleAction * action, GVariant * parameter, gpointer user_data)
{
  ZrythmApp * self = ZRYTHM_APP (user_data);

  greeter_widget_set_progress_and_status (
    *self->greeter, "", _ ("Loading Project"), 0.99);

  z_debug (
    "on_load_project called | open filename '{}' | opening template {}",
    gZrythm->open_filename_, gZrythm->opening_template_);
  g_application_hold (g_application_get_default ());
  zrythm_app->project_init_flow_mgr = std::make_unique<ProjectInitFlowManager> (
    gZrythm->open_filename_, gZrythm->opening_template_,
    project_load_or_create_ready_cb, nullptr);
}

static void
on_plugin_scan_finished (ZrythmApp * self)
{
  z_info ("plugin scan finished");
  self->init_finished = true;
}

void *
zrythm_app_init_thread (ZrythmApp * self)
{
  z_info ("init thread starting...");

  // sleep for a while otherwise there is a weird issue with the greeter (no idea
  // what the cause is but it thinks it's NULL inside the function call below)
  std::this_thread::sleep_for (std::chrono::milliseconds (00));

  greeter_widget_set_progress_and_status (
    *self->greeter, _ ("Initializing"), _ ("Initializing settings"), 0.0);

  /* init zrythm folders ~/Zrythm */
  auto msg = format_str (_ ("Initializing {} directories"), PROGRAM_NAME);
  greeter_widget_set_progress_and_status (
    *self->greeter, "", msg.c_str (), 0.01);
  try
    {
      gZrythm->init_user_dirs_and_files ();
    }
  catch (const ZrythmException &e)
    {
      z_critical ("Failed to create user dirs and files: {}", e.what ());
      return nullptr;
    }
  init_recent_projects ();
  gZrythm->init_templates ();

  /* init log */
  greeter_widget_set_progress_and_status (
    *self->greeter, "", _ ("Initializing logging system"), 0.02);
// TODO:
#if 0
  success = log_init_with_file (LOG, nullptr, &err);
  if (!success)
    {
      g_error ("Failed to init log with file: %s", err->message);
      return NULL;
    }
#endif

  {
    char ver[2000];
    Zrythm::get_version_with_capabilities (ver, false);
    z_info ("\n{}", ver);
  }

#if defined(_WIN32) || defined(__APPLE__)
  z_warning (
    "Warning, you are running a non-free operating "
    "system.");
#endif

  greeter_widget_set_progress_and_status (
    *self->greeter, "", _ ("Initializing caches"), 0.05);
  self->ui_caches = std::make_unique<UiCaches> ();

  greeter_widget_set_progress_and_status (
    *self->greeter, "", _ ("Initializing file manager"), 0.06);
  gZrythm->get_file_manager ().load_files ();

  greeter_widget_set_progress_and_status (
    *self->greeter, _ ("Scanning Plugins"), _ ("Scanning Plugins"), 0.10);
  gZrythm->plugin_manager_->begin_scan (
    0.90, &self->greeter->progress,
    [self] () { on_plugin_scan_finished (self); });

  return nullptr;
}

int
zrythm_app_prompt_for_project_func (ZrythmApp * self)
{
  if (self->init_finished)
    {
      // TODO: replace
      // log_init_writer_idle (LOG, 3);

      g_action_group_activate_action (
        G_ACTION_GROUP (self), "prompt_for_project", nullptr);

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Called before on_load_project.
 *
 * Checks if a project was given in the command line. If not, it prompts the
 * user for a project.
 */
static void
on_prompt_for_project (GSimpleAction * action, GVariant * parameter, gpointer data)
{
  z_info ("prompting for project...");

  ZrythmApp * self = zrythm_app.get ();

  if (!gZrythm->open_filename_.empty ())
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (self), "load_project", nullptr);
    }
  else
    {
      /* if running for the first time (even after the GSetting is set to false)
       * run the demo project, otherwise ask the user for a project */
      bool use_demo_template =
        self->is_first_run && !gZrythm->demo_template_.empty ();
#ifdef _WIN32
      /* crashes on windows -- fix first then re-enable */
      use_demo_template = false;
#endif

      /* disable this functionality on all platforms for now - it causes issues
       * with triplesynth and complicates the greeter flow */
      use_demo_template = false;

      greeter_widget_select_project (
        self->greeter, false, false,
        use_demo_template ? gZrythm->demo_template_.c_str () : nullptr);

#ifdef __APPLE__
      /* possibly not necessary / working, forces app window on top */
      /*show_on_top ();*/
#endif
    }

  z_info ("done");
}

void
zrythm_app_set_font_scale (ZrythmApp * self, double font_scale)
{
  /* this was taken from the GtkInspector
   * visual.c code */
  g_object_set (
    self->default_settings, "gtk-xft-dpi", (int) (font_scale * 96 * 1024),
    nullptr);
}

/*
 * Called after startup if no filename is passed on command line.
 */
static void
zrythm_app_activate (GApplication * _app)
{
  z_info ("Activating...");
  /*z_info ("activate {}", *task_id);*/

  z_info ("done");
}

/**
 * Called when a filename is passed to the command line instead of activate.
 *
 * Always gets called after startup and before the tasks.
 */
static void
zrythm_app_open (
  GApplication * _app,
  GFile **       files,
  gint           n_files,
  const gchar *  hint)
{
  z_info ("Opening...");

  z_warn_if_fail (n_files == 1);

  GFile * file = files[0];
  auto    path = g_file_get_path (file);
  gZrythm->open_filename_ = path;
  g_free (path);
  z_info ("open {}", gZrythm->open_filename_);

  z_info ("done");
}

void
zrythm_app_install_action_accel (
  ZrythmApp *  self,
  const char * primary,
  const char * secondary,
  const char * action_name)
{
  z_warn_if_fail (zrythm_app);
  const char * accels[] = { primary, secondary, NULL };
  gtk_application_set_accels_for_action (
    GTK_APPLICATION (zrythm_app.get ()), action_name, accels);
}

char *
zrythm_app_get_primary_accel_for_action (
  ZrythmApp *  self,
  const char * action_name)
{
  z_warn_if_fail (zrythm_app);
  guint           accel_key;
  GdkModifierType accel_mods;
  gchar **        accels = gtk_application_get_accels_for_action (
    GTK_APPLICATION (zrythm_app.get ()), action_name);
  char * ret = NULL;
  if (accels[0] != nullptr)
    {
      gtk_accelerator_parse (accels[0], &accel_key, &accel_mods);
      ret = gtk_accelerator_get_label (accel_key, accel_mods);
    }
  g_strfreev (accels);
  return ret;
}

static void
print_gdk_pixbuf_format_info (gpointer data, gpointer user_data)
{
  GdkPixbufFormat * format = (GdkPixbufFormat *) data;
  ZrythmApp *       self = ZRYTHM_APP (user_data);

  char *  name = gdk_pixbuf_format_get_name (format);
  char *  description = gdk_pixbuf_format_get_description (format);
  char *  license = gdk_pixbuf_format_get_license (format);
  char    mime_types[800] = "";
  char ** _mime_types = gdk_pixbuf_format_get_mime_types (format);
  char *  tmp = NULL;
  int     i = 0;
  while ((tmp = _mime_types[i++]))
    {
      strcat (mime_types, tmp);
      strcat (mime_types, ", ");
    }
  mime_types[strlen (mime_types) - 2] = '\0';
  g_strfreev (_mime_types);
  char    extensions[800] = "";
  char ** _extensions = gdk_pixbuf_format_get_extensions (format);
  tmp = NULL;
  i = 0;
  while ((tmp = _extensions[i++]))
    {
      strcat (extensions, tmp);
      strcat (extensions, ", ");
      if (g_str_has_prefix (tmp, "svg"))
        {
          self->have_svg_loader = true;
        }
    }
  extensions[strlen (extensions) - 2] = '\0';
  g_strfreev (_extensions);
  z_info (
    "Found GDK Pixbuf Format:\n"
    "name: {}\ndescription: {}\n"
    "mime types: {}\nextensions: {}\n"
    "is scalable: {}\nis disabled: {}\n"
    "license: {}",
    name, description, mime_types, extensions,
    gdk_pixbuf_format_is_scalable (format),
    gdk_pixbuf_format_is_disabled (format), license);
  g_free (name);
  g_free (description);
  g_free (license);
}

static void
load_icon (
  GtkSettings *  default_settings,
  GtkIconTheme * icon_theme,
  const char *   icon_name)
{
  z_info (
    "Attempting to load an icon from the icon "
    "theme...");
  bool found = gtk_icon_theme_has_icon (icon_theme, icon_name);
  z_info ("found: {}", found);
  if (!gtk_icon_theme_has_icon (icon_theme, icon_name))
    {
      /* fallback to zrythm-dark and try again */
      z_warning (
        "icon '{}' not found, falling back to "
        "zrythm-dark",
        icon_name);
      g_object_set (
        default_settings, "gtk-icon-theme-name", "zrythm-dark", nullptr);
      found = gtk_icon_theme_has_icon (icon_theme, icon_name);
      z_info ("found: {}", found);
    }

  if (!found)
    {
      char err_msg[600];
      strcpy (
        err_msg,
        "Failed to load icon from icon theme. "
        "Please install zrythm.");
      z_error ("{}", err_msg);
      fprintf (stderr, "%s\n", err_msg);
      ui_show_message_literal (_ ("Icon Theme Not Found"), err_msg);
      z_error ("Failed to load icon");
    }
  z_info ("Icon found.");
}

static void
lock_memory (ZrythmApp * self)
{
#ifdef _WIN32
  /* TODO */
#else

  bool have_unlimited_mem = false;

  struct rlimit rl;
  if (getrlimit (RLIMIT_MEMLOCK, &rl) == 0)
    {
      if (rl.rlim_max == RLIM_INFINITY)
        {
          if (rl.rlim_cur == RLIM_INFINITY)
            {
              have_unlimited_mem = true;
            }
          else
            {
              rl.rlim_cur = RLIM_INFINITY;
              if (setrlimit (RLIMIT_MEMLOCK, &rl) == 0)
                {
                  have_unlimited_mem = true;
                }
              else
                {
                  ZrythmAppUiMessage * ui_msg = zrythm_app_ui_message_new (
                    GTK_MESSAGE_ERROR,
                    "Could not set system memory "
                    "lock limit to 'unlimited'");
                  g_async_queue_push (self->project_load_message_queue, ui_msg);
                }
            }
        }
      else
        {
          char * str = g_strdup_printf (
            _ ("Your user does not have enough "
               "privileges to allow %s to lock "
               "unlimited memory. This may cause "
               "audio dropouts. Please refer to "
               "the 'Getting Started' section in "
               "the user manual for details."),
            PROGRAM_NAME);
          ZrythmAppUiMessage * ui_msg =
            zrythm_app_ui_message_new (GTK_MESSAGE_WARNING, str);
          g_async_queue_push (self->project_load_message_queue, ui_msg);
          g_free (str);
        }
    }
  else
    {
      auto str = format_str (
        _ ("Could not get system memory lock limit ({})"), strerror (errno));
      ZrythmAppUiMessage * ui_msg =
        zrythm_app_ui_message_new (GTK_MESSAGE_WARNING, str.c_str ());
      g_async_queue_push (self->project_load_message_queue, ui_msg);
    }

  if (have_unlimited_mem)
    {
      /* lock down memory */
      z_info ("Locking down memory...");
      if (mlockall (MCL_CURRENT | MCL_FUTURE))
        {
#  ifdef __APPLE__
          z_warning ("Cannot lock down memory: {}", strerror (errno));
#  else
          char * str = g_strdup_printf (
            _ ("Cannot lock down memory: %s"), strerror (errno));
          ZrythmAppUiMessage * ui_msg =
            zrythm_app_ui_message_new (GTK_MESSAGE_WARNING, str);
          g_async_queue_push (self->project_load_message_queue, ui_msg);
          g_free (str);
#  endif
        }
    }
#endif
}

/**
 * lotsa_files_please() from ardour
 * (libs/ardour/globals.cc).
 */
static void
raise_open_file_limit (void)
{
#ifdef _WIN32
  /* this only affects stdio. 2048 is the maximum possible (512 the default).
   *
   * If we want more, we'll have to replaces the POSIX I/O interfaces with
   * Win32 API calls (CreateFile, WriteFile, etc) which allows for 16K.
   *
   * see
   * http://stackoverflow.com/questions/870173/is-there-a-limit-on-number-of-open-files-in-windows
   * and http://bugs.mysql.com/bug.php?id=24509
   */
  int newmax = _setmaxstdio (2048);
  if (newmax > 0)
    {
      z_info (
        "Your system is configured to limit %s to "
        "{} open files",
        PROGRAM_NAME, newmax);
    }
  else
    {
      z_warning (
        "Could not set system open files limit. "
        "Current limit is {} open files",
        _getmaxstdio ());
    }
#else /* else if not _WIN32 */
  struct rlimit rl;

  if (getrlimit (RLIMIT_NOFILE, &rl) == 0)
    {
#  ifdef __APPLE__
      /* See the COMPATIBILITY note on the Apple setrlimit() man page */
      rl.rlim_cur = MIN ((rlim_t) OPEN_MAX, rl.rlim_max);
#  else
      rl.rlim_cur = rl.rlim_max;
#  endif

      if (setrlimit (RLIMIT_NOFILE, &rl) != 0)
        {
          if (rl.rlim_cur == RLIM_INFINITY)
            {

              z_warning (
                "Could not set system open files limit to \"unlimited\"");
            }
          else
            {
              z_warning (
                "Could not set system open files limit to {}",
                (uintmax_t) rl.rlim_cur);
            }
        }
      else
        {
          if (rl.rlim_cur != RLIM_INFINITY)
            {
              z_info (
                "Your system is configured to limit {} to {} open files",
                PROGRAM_NAME, (uintmax_t) rl.rlim_cur);
            }
        }
    }
  else
    {
      z_warning ("Could not get system open files limit ({})", strerror (errno));
    }
#endif
}

/**
 * First function that gets called afted CLI args are parsed and processed.
 *
 * This gets called before open or activate.
 */
static void
zrythm_app_startup (GApplication * app)
{
  z_info ("Starting up...");

  ZrythmApp * self = ZRYTHM_APP (app);

  Backtrace::init_signal_handlers ();

  char * exe_path = NULL;
  int    dirname_length, length;
  length = wai_getExecutablePath (nullptr, 0, &dirname_length);
  if (length > 0)
    {
      exe_path = (char *) malloc ((size_t) length + 1);
      wai_getExecutablePath (exe_path, length, &dirname_length);
      exe_path[length] = '\0';
    }

  Zrythm::getInstance ()->pre_init (
    exe_path ? exe_path : self->argv[0], true, true);

  /* init localization, using system locale if first run */
  GSettings * prefs = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".general");
  self->is_first_run = g_settings_get_boolean (prefs, "first-run");
  localization_init (self->is_first_run, true, true);
  g_object_unref (G_OBJECT (prefs));

  gZrythm->init ();

  const char * copyright_line =
    "Copyright (C) " COPYRIGHT_YEARS " " COPYRIGHT_NAME;
  char * ver = Zrythm::get_version (0);
  fprintf (
    stdout,
    _ ("%s-%s\n%s\n\n"
       "%s comes with ABSOLUTELY NO WARRANTY!\n\n"
       "This is free software, and you are welcome to redistribute it\n"
       "under certain conditions. See the file 'COPYING' for details.\n\n"
       "Write comments and bugs to %s\n"
       "Support this project at https://liberapay.com/Zrythm\n\n"),
    PROGRAM_NAME, ver, copyright_line, PROGRAM_NAME, ISSUE_TRACKER_URL);
  g_free (ver);

  char * cur_dir = g_get_current_dir ();
  z_info ("Running Zrythm in {}", cur_dir);
  g_free (cur_dir);

  z_info ("GTK_THEME was '{}'. unsetting...", Glib::getenv ("GTK_THEME"));
  Glib::unsetenv ("GTK_THEME");

  /* install segfault handler */
  z_info ("Installing signal handlers...");
#ifdef HAVE_VALGRIND
  if (!RUNNING_ON_VALGRIND)
    {
#endif
      signal (SIGSEGV, segv_handler);
      signal (SIGABRT, segv_handler);
      signal (SIGTERM, sigterm_handler);
#ifdef HAVE_VALGRIND
    }
#endif

#ifdef HAVE_X11
  /* init xlib threads */
  z_info ("Initing X threads...");
  XInitThreads ();
#endif

  /* init suil */
  /*z_info ("Initing suil...");*/
  /*suil_init (&self->argc, &self->argv, SUIL_ARG_NONE);*/

  /* init fftw */
  z_info ("Making fftw planner thread safe...");
  fftw_make_planner_thread_safe ();
  fftwf_make_planner_thread_safe ();

  /* init curl */
  curl_global_init (CURL_GLOBAL_ALL);

  /* init libpanel */
  panel_init ();

  /* init libadwaita */
  /*adw_init (); already called by panel_init () */
  AdwStyleManager * style_mgr = adw_style_manager_get_default ();
  adw_style_manager_set_color_scheme (style_mgr, ADW_COLOR_SCHEME_FORCE_DARK);

  /* init gtksourceview */
  gtk_source_init ();
  z_gtk_source_language_manager_get ();

  G_APPLICATION_CLASS (zrythm_app_parent_class)->startup (G_APPLICATION (self));

  z_info ("called startup on G_APPLICATION_CLASS");

  bool ret = g_application_get_is_registered (G_APPLICATION (self));
  bool remote = g_application_get_is_remote (G_APPLICATION (self));
  z_info ("application registered: {}, is remote {}", ret, remote);

  z_info (
    "application resources base path: {}",
    g_application_get_resource_base_path (G_APPLICATION (app)));

  self->default_settings = gtk_settings_get_default ();

  /* set theme */
#if 0
  g_object_set (
    self->default_settings, "gtk-theme-name",
    "Matcha-dark-sea", nullptr);
  g_object_set (
    self->default_settings,
    "gtk-application-prefer-dark-theme", 1, nullptr);
#endif
  int scale_factor = z_gtk_get_primary_monitor_scale_factor ();
  z_info ("Monitor scale factor: {}", scale_factor);
#if defined(_WIN32)
  g_object_set (
    self->default_settings, "gtk-font-name", "Segoe UI Normal 10", nullptr);
  g_object_set (
    self->default_settings, "gtk-cursor-theme-name", "Adwaita", nullptr);
#elif defined(__APPLE__)
  g_object_set (self->default_settings, "gtk-font-name", "Regular 10", nullptr);
#else
  g_object_set (
    self->default_settings, "gtk-font-name", "Cantarell Regular 10", nullptr);
#endif

  /* explicitly set font scaling */
  double font_scale = g_settings_get_double (S_P_UI_GENERAL, "font-scale");
  zrythm_app_set_font_scale (self, font_scale);

  z_info ("Theme set");

  GtkIconTheme * icon_theme = z_gtk_icon_theme_get_default ();
  char * icon_theme_name = g_settings_get_string (S_P_UI_GENERAL, "icon-theme");
  z_info ("setting icon theme to '{}'", icon_theme_name);
  g_object_set (
    self->default_settings, "gtk-icon-theme-name", icon_theme_name, nullptr);
  g_free_and_null (icon_theme_name);

  /* --- add icon search paths --- */

  /* prepend freedesktop system icons to search path, just in case */
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  auto parent_datadir = dir_mgr->get_dir (ZrythmDirType::SYSTEM_PARENT_DATADIR);
  auto freedesktop_icon_theme_dir =
    Glib::build_filename (parent_datadir, "icons");
  gtk_icon_theme_add_search_path (
    icon_theme, freedesktop_icon_theme_dir.c_str ());
  z_debug ("added icon theme search path: {}", freedesktop_icon_theme_dir);

  /* prepend zrythm system icons to search path */
  {
    auto system_icon_theme_dir =
      dir_mgr->get_dir (ZrythmDirType::SYSTEM_THEMES_ICONS_DIR);
    gtk_icon_theme_add_search_path (icon_theme, system_icon_theme_dir.c_str ());
    z_debug ("added icon theme search path: {}", system_icon_theme_dir);
  }

  /* prepend user custom icons to search path */
  {
    auto user_icon_theme_dir =
      dir_mgr->get_dir (ZrythmDirType::USER_THEMES_ICONS);
    gtk_icon_theme_add_search_path (icon_theme, user_icon_theme_dir.c_str ());
    z_debug ("added icon theme search path: {}", user_icon_theme_dir);
  }

  /* --- end icon paths --- */

  /* look for found loaders */
  z_info ("looking for GDK Pixbuf formats...");
  GSList * formats_list = gdk_pixbuf_get_formats ();
  g_slist_foreach (formats_list, print_gdk_pixbuf_format_info, self);
  g_slist_free (g_steal_pointer (&formats_list));
  if (!self->have_svg_loader)
    {
      fprintf (stderr, "SVG loader was not found.\n");
      exit (-1);
    }

  /* try to load some icons */
  /* zrythm */
  load_icon (self->default_settings, icon_theme, "solo");

  z_info ("Setting gtk icon theme resource paths...");
  /* TODO auto-generate this code from meson (also in gen-gtk-resources-xml
   * script) */
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/arena");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/gnome-builder");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/gnome-icon-library");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/fluentui");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/jam-icons");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/box-icons");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/iconpark");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/iconoir");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/material-design");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/untitled-ui");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/css.gg");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/codicons");

  z_info ("Resource paths set");

  /* get css theme file path */
  GtkCssProvider * css_provider = gtk_css_provider_new ();
  auto   user_themes_dir = dir_mgr->get_dir (ZrythmDirType::USER_THEMES_CSS);
  char * css_theme_file = g_settings_get_string (S_P_UI_GENERAL, "css-theme");
  auto css_theme_path = Glib::build_filename (user_themes_dir, css_theme_file);
  if (!Glib::file_test (css_theme_path, Glib::FileTest::EXISTS))
    {
      /* fallback to theme in system path */
      auto system_themes_dir =
        dir_mgr->get_dir (ZrythmDirType::SYSTEM_THEMES_CSS_DIR);
      css_theme_path = Glib::build_filename (system_themes_dir, css_theme_file);
    }
  if (!Glib::file_test (css_theme_path, Glib::FileTest::EXISTS))
    {
      /* fallback to zrythm-theme.css */
      auto system_themes_dir =
        dir_mgr->get_dir (ZrythmDirType::SYSTEM_THEMES_CSS_DIR);
      css_theme_path =
        Glib::build_filename (system_themes_dir, "zrythm-theme.css");
    }
  g_free (css_theme_file);
  z_info ("CSS theme path: {}", css_theme_path);

  /* set default css provider */
  gtk_css_provider_load_from_path (css_provider, css_theme_path.c_str ());
  gtk_style_context_add_provider_for_display (
    gdk_display_get_default (), GTK_STYLE_PROVIDER (css_provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css_provider);
  z_info ("set default css provider from path: {}", css_theme_path);

  /* set default window icon */
  gtk_window_set_default_icon_name ("zrythm");

  /* allow maximum number of open files (taken from
   * ardour) */
  raise_open_file_limit ();

  lock_memory (self);

  /* print detected vamp plugins */
  vamp_print_all ();

  self->greeter = greeter_widget_new (self, nullptr, true, false);
  gtk_window_present (GTK_WINDOW (self->greeter));

  /* install accelerators for each action */
  std::string primary;
  std::string secondary;

  // Lambda to install accelerator with secondary keybinding
  auto install_accel_with_secondary =
    [&] (const auto &keybind, const auto &secondary_keybind, const auto &action) {
      primary = S_USER_SHORTCUTS.get (true, action, keybind);
      secondary = S_USER_SHORTCUTS.get (false, action, secondary_keybind);
      zrythm_app_install_action_accel (
        self, primary.c_str (), secondary.c_str (), action);
    };

  // Lambda to install accelerator with optional secondary keybinding
  auto install_accel = [&] (const auto &keybind, const auto &action) {
    return install_accel_with_secondary (keybind, "", action);
  };

  install_accel ("F1", "app.manual");
  install_accel ("<Control>q", "app.quit");
  install_accel ("F6", "app.cycle-focus");
  install_accel ("<Shift>F6", "app.cycle-focus-backwards");
  install_accel ("F10", "app.focus-first-widget");
  install_accel ("F11", "app.fullscreen");
  install_accel ("<Control>comma", "app.preferences");
  install_accel ("<Control>n", "app.new");
  install_accel ("<Control>o", "app.open");
  install_accel ("<Control>s", "app.save");
  install_accel ("<Control><Shift>s", "app.save-as");
  install_accel ("<Control>e", "app.export-as");
  install_accel ("<Control>z", "app.undo");
  install_accel ("<Control><Shift>z", "app.redo");
  install_accel ("<Control>x", "app.cut");
  install_accel ("<Control>c", "app.copy");
  install_accel ("<Control>v", "app.paste");
  install_accel ("<Control>d", "app.duplicate");
  install_accel ("<Shift>F10", "app.toggle-left-panel");
  install_accel ("<Shift>F12", "app.toggle-right-panel");
  install_accel ("<Shift>F11", "app.toggle-bot-panel");
  install_accel_with_secondary (
    "<Control>equal", "<Control>KP_Add", "app.zoom-in::global");
  install_accel_with_secondary (
    "<Control>minus", "<Control>KP_Subtract", "app.zoom-out::global");
  install_accel_with_secondary (
    "<Control>plus", "<Control>0", "app.original-size::global");
  install_accel ("<Control>bracketleft", "app.best-fit::global");
  install_accel ("<Control>l", "app.loop-selection");
  install_accel ("Home", "app.goto-start-marker");
  install_accel ("End", "app.goto-end-marker");
  install_accel_with_secondary (
    "Page_Up", "<Control>BackSpace", "app.goto-prev-marker");
  install_accel ("Page_Down", "app.goto-next-marker");
  install_accel ("<Control>space", "app.play-pause");
  install_accel ("<Alt>Q", "app.quantize-options::global");
  install_accel ("<Control>J", "app.merge-selection");
  install_accel (gdk_keyval_name (GDK_KEY_Home), "app.go-to-start");

#undef INSTALL_ACCEL

  z_info ("done");
}

/**
 * Called immediately after the main GTK loop
 * terminates.
 *
 * This is also called manually on SIGINT.
 */
static void
zrythm_app_on_shutdown (GApplication * application, ZrythmApp * self)
{
  z_info ("Shutting down...");

  if (self->project_autosave_source_id != 0)
    {
      g_source_remove (self->project_autosave_source_id);
      self->project_autosave_source_id = 0;
    }

  if (gZrythm)
    {
      if (gZrythm->project_ && gZrythm->project_->audio_engine_)
        {
          gZrythm->project_->audio_engine_->activate (false);
        }
      gZrythm->project_.reset ();
      gZrythm->deleteInstance ();
    }

  object_free_w_func_and_null (
    g_async_queue_unref, self->project_load_message_queue);

  gtk_source_finalize ();

  std::destroy_at (&self->ui_caches);
  std::destroy_at (&self->project_init_flow_mgr);

  if (self->default_settings)
    {
      g_object_unref (self->default_settings);
    }

  /* init curl */
  curl_global_cleanup ();

  ZrythmDirectoryManager::deleteInstance ();
  PCGRand::deleteInstance ();
  GSettingsManager::deleteInstance ();

  z_info ("done shutting down - finally deleting logger...");
  Logger::deleteInstance ();
}

#if 0
/**
 * Checks that the output is not NULL and exits if it is.
 */
static void
verify_output_exists (ZrythmApp * self)
{
  if (!self->output_file)
    {
      char str[600];
      sprintf (
        str, "%s\n",
        _ ("An output file was not specified. Please pass one with `--output=FILE`."));
      fprintf (stderr, "%s", str);
      exit (EXIT_FAILURE);
    }
}
#endif

static bool
print_settings (ZrythmApp * self)
{
  localization_init (false, false, false);
  GSettingsManager::print_all_settings (self->pretty_print);

  exit (EXIT_SUCCESS);
}

static bool
reset_to_factory (void)
{
  GSettingsManager::reset_to_factory (true, true);

  exit (EXIT_SUCCESS);
}

/**
 * Called on the local instance to handle options.
 */
static int
on_handle_local_options (GApplication * app, GVariantDict * opts, ZrythmApp * self)
{
  /*z_debug ("handling options");*/

#if 0
  /* print the contents */
  GVariant * variant = g_variant_dict_end (opts);
  char * str = g_variant_print (variant, true);
  z_warning("%s", str);
#endif

  if (g_variant_dict_contains (opts, "print-settings"))
    {
      print_settings (self);
    }
  else if (g_variant_dict_contains (opts, "reset-to-factory"))
    {
      reset_to_factory ();
    }

#ifdef APPIMAGE_BUILD
  {
    char * rpath = NULL;
    if (!g_variant_dict_lookup (opts, "appimage-runtime-path", "&s", &rpath))
      {
        g_error ("must pass --appimage-runtime-path");
      }
    self->appimage_runtime_path = g_strdup (rpath);
  }
#endif

  return -1;
}

static bool
print_version (
  const gchar * option_name,
  const gchar * value,
  gpointer      data,
  GError **     error)
{
  char ver_with_caps[2000];
  Zrythm::get_version_with_capabilities (ver_with_caps, false);
  fprintf (
    stdout, "%s\n%s\n%s\n%s\n", ver_with_caps,
    "Copyright © " COPYRIGHT_YEARS " " COPYRIGHT_NAME,
    "This is free software; see the source for copying conditions.",
    "There is NO "
    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

  exit (EXIT_SUCCESS);
}

static bool
set_dummy (
  const gchar * option_name,
  const gchar * value,
  gpointer      data,
  GError **     error)
{
  zrythm_app->midi_backend = g_strdup ("Dummy");
  zrythm_app->audio_backend = g_strdup ("Dummy");

  return true;
}

/**
 * Add the option entries.
 *
 * Things that can be processed immediately should
 * be set as callbacks here (like --version).
 *
 * Things that require to know other options before
 * running should be set as NULL and processed
 * in the handle-local-options handler.
 */
static void
add_option_entries (ZrythmApp * self)
{
  GOptionEntry entries[] = {
    { "version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     (gpointer) print_version, _ ("Print version information"), NULL },
    { "pretty", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &self->pretty_print,
     _ ("Print output in user-friendly way"), NULL },
    { "print-settings", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, nullptr,
     _ ("Print current settings"), NULL },
    { "reset-to-factory", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, nullptr,
     _ ("Reset to factory settings"), NULL },
    { "audio-backend", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
     &self->audio_backend, _ ("Override the audio backend to use"), "BACKEND" },
    { "midi-backend", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
     &self->midi_backend, _ ("Override the MIDI backend to use"), "BACKEND" },
    { "dummy", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     (gpointer) set_dummy,
     _ ("Shorthand for --midi-backend=none "
         "--audio-backend=none"),
     NULL },
    { "buf-size", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &self->buf_size,
     "Override the buffer size to use for the "
      "audio backend, if applicable", "BUF_SIZE" },
    { "samplerate", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &self->samplerate,
     "Override the samplerate to use for the "
      "audio backend, if applicable", "SAMPLERATE" },
    { "output", 'o', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
     &self->output_file, "File or directory to output to", "FILE" },
#ifdef APPIMAGE_BUILD
    { "appimage-runtime-path", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
     nullptr, "AppImage runtime path", "PATH" },
#endif
    {},
  };

  g_application_add_main_option_entries (G_APPLICATION (self), entries);
  g_application_set_option_context_parameter_string (
    G_APPLICATION (self), _ ("[PROJECT-FILE]"));

  char examples[8000];
  sprintf (
    examples,
    _ ("Examples:\n"
       "  -p --pretty                         Pretty-print current settings\n\n"
       "Please report issues to %s\n"),
    ISSUE_TRACKER_URL);
  g_application_set_option_context_description (G_APPLICATION (self), examples);

  char summary[8000];
  sprintf (
    summary, _ ("Run %s, optionally passing a project file."), PROGRAM_NAME);
  g_application_set_option_context_summary (G_APPLICATION (self), summary);
}

/**
 * Shows the trial limitation error message.
 *
 * @return Whether the limit was reached.
 */
bool
zrythm_app_check_and_show_trial_limit_error (ZrythmApp * self)
{
#ifdef TRIAL_VER
  if (PROJECT && TRACKLIST && TRACKLIST->tracks.size () >= TRIAL_MAX_TRACKS)
    {
      ui_show_error_message_printf (
        _ ("Track Limitation"),
        _ ("This version of Zrythm does not support creating more than %d tracks."),
        TRIAL_MAX_TRACKS);
      return true;
    }
#endif

  return false;
}

void
zrythm_exit_response_callback (AdwDialog * dialog, gpointer user_data)
{
  exit (EXIT_SUCCESS);
}

/**
 * Creates the Zrythm GApplication.
 *
 * This also initializes the Zrythm struct.
 */
std::unique_ptr<ZrythmApp, ZrythmAppDeleter>
zrythm_app_new (int argc, const char ** argv)
{
  auto * self = static_cast<ZrythmApp *> (g_object_new (
    ZRYTHM_APP_TYPE,
    /* if an ID is provided, this application
     * becomes unique (only one instance allowed) */
    /*"application-id", "org.zrythm.Zrythm",*/
    "resource-base-path", "/org/zrythm/Zrythm", "flags",
    G_APPLICATION_HANDLES_OPEN, nullptr));

  // zrythm_app.reset (self);

  // self->gtk_thread = g_thread_self ();
  self->gtk_thread_id = current_thread_id.get ();

  self->project_load_message_queue = g_async_queue_new ();

  /* add option entries */
  add_option_entries (self);

  /* add handlers */
  g_signal_connect (
    G_OBJECT (self), "shutdown", G_CALLBACK (zrythm_app_on_shutdown), self);
  g_signal_connect (
    G_OBJECT (self), "handle-local-options",
    G_CALLBACK (on_handle_local_options), self);

  return std::unique_ptr<ZrythmApp, ZrythmAppDeleter> (self);
}

static void
finalize (ZrythmApp * self)
{
  // note: this function doesn't get called in normal (not testing) mode -
  // it gets called when ZrythmAppDeleter goes in effect, at the end of exit()
  // because it is a global variable
  // (the logger is already deleted so don't attempt to log here)
  // z_info ("finalizing ZrythmApp...");
  // z_info ("{}: done", __func__);
}

static void
zrythm_app_class_init (ZrythmAppClass * klass)
{
  G_APPLICATION_CLASS (klass)->activate = zrythm_app_activate;
  G_APPLICATION_CLASS (klass)->startup = zrythm_app_startup;
  G_APPLICATION_CLASS (klass)->open = zrythm_app_open;
  /*G_APPLICATION_CLASS (klass)->shutdown =*/
  /*zrythm_app_shutdown;*/

  GObjectClass * goklass = G_OBJECT_CLASS (klass);

  goklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
zrythm_app_init (ZrythmApp * self)
{
  std::construct_at (&self->ui_caches);
  std::construct_at (&self->project_init_flow_mgr);

  gdk_set_allowed_backends (
    /* 1. prefer native backends on windows/mac (avoid x11)
     * 2. Some plugins crash on wayland:
     * https://github.com/falkTX/Carla/issues/1527
     * so there used to be "x11,wayland" here but GTK
     * misbehaving on X11 caused more issues so the current
     * solution is to bridge all plugins by default
     */
    "quartz,win32,*");

  const GActionEntry entries[] = {
    { "prompt_for_project", on_prompt_for_project },
    /*{ "init_main_window", on_init_main_window },*/
    { "setup_main_window",  on_setup_main_window  },
    { "load_project",       on_load_project       },
    { "about",              activate_about        },
    { "fullscreen",         activate_fullscreen   },
    { "chat",               activate_chat         },
    { "manual",             activate_manual       },
    { "news",               activate_news         },
    { "bugreport",          activate_bugreport    },
    { "donate",             activate_donate       },
    { "minimize",           activate_minimize     },
    // { "log",               activate_log         },
    { "preferences",        activate_preferences  },
    { "quit",               activate_quit         },
  };

  g_action_map_add_action_entries (
    G_ACTION_MAP (self), entries, G_N_ELEMENTS (entries), self);
}
