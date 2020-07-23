/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#ifndef _WOE32
#include <sys/mman.h>
#endif
#include <stdlib.h>

#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "audio/engine.h"
#include "audio/router.h"
#include "audio/quantize_options.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/first_run_assistant.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/project_assistant.h"
#include "gui/widgets/splash.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/env.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "Wrapper.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef HAVE_GTK_SOURCE_VIEW_4
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop
#endif

/** This is declared extern in zrythm_app.h. */
ZrythmApp * zrythm_app = NULL;

G_DEFINE_TYPE (
  ZrythmApp, zrythm_app,
  GTK_TYPE_APPLICATION);

/**
 * Initializes/creates the default dirs/files.
 */
static void
init_dirs_and_files ()
{
  g_message ("initing dirs and files");
  char * dir;

#define MK_USER_DIR(x) \
  dir =  zrythm_get_dir (ZRYTHM_DIR_USER_##x); \
  io_mkdir (dir); \
  g_free (dir)

  MK_USER_DIR (TOP);
  MK_USER_DIR (PROJECTS);
  MK_USER_DIR (TEMPLATES);
  MK_USER_DIR (LOG);
  MK_USER_DIR (THEMES);
  MK_USER_DIR (PROFILING);
  MK_USER_DIR (GDB);

#undef MK_USER_DIR
}

/**
 * Initializes the array of recent projects in
 * Zrythm app.
 */
static void
init_recent_projects ()
{
  g_message ("Initializing recent projects...");

  gchar ** recent_projects =
    g_settings_get_strv (
      S_GENERAL, "recent-projects");

  /* get recent projects */
  ZRYTHM->num_recent_projects = 0;
  int count = 0;
  char * prj;
  while (recent_projects[count])
    {
      prj = recent_projects[count];

      /* skip duplicates */
      if (array_contains_cmp (
             ZRYTHM->recent_projects,
             ZRYTHM->num_recent_projects,
             prj, strcmp, 0, 1))
        {
          count++;
          continue;
        }

      ZRYTHM->recent_projects[
        ZRYTHM->num_recent_projects++] =
          g_strdup (prj);
    }
  g_strfreev (recent_projects);

  /* set last element to NULL because the call
   * takes a NULL terminated array */
  ZRYTHM->recent_projects[
    ZRYTHM->num_recent_projects] = NULL;

  /* save the new list */
  g_settings_set_strv (
    S_GENERAL, "recent-projects",
    (const char * const *) ZRYTHM->recent_projects);

  g_message ("done");
}

/**
 * Initializes the array of recent projects in
 * Zrythm app.
 */
static void
init_templates ()
{
  g_message ("Initializing templates...");

  char * user_templates_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_TEMPLATES);
  ZRYTHM->templates =
    io_get_files_in_dir (user_templates_dir);
  g_free (user_templates_dir);

  g_message ("done");
}

/**
 * Called after the main window and the project have been
 * initialized. Sets up the window using the backend.
 *
 * This is the final step.
 */
static void
on_setup_main_window (
  GSimpleAction * action,
  GVariant *      parameter,
  gpointer        data)
{
  g_message ("setting up...");

  ZrythmApp * self = ZRYTHM_APP (data);

  zrythm_app_set_progress_status (
    self, _("Setting up main window"), 0.98);

  /*main_window_widget_setup (MAIN_WINDOW);*/

  /*router_recalc_graph (ROUTER);*/
  /*engine_set_run (AUDIO_ENGINE, true);*/

#ifndef TRIAL_VER
  /* add timeout for auto-saving projects */
  unsigned int autosave_interval =
    g_settings_get_uint (
      S_P_PROJECTS_GENERAL, "autosave-interval");
  if (autosave_interval > 0)
    {
      PROJECT->last_autosave_time =
        g_get_monotonic_time ();
      g_timeout_add_seconds (
        3, project_autosave_cb, NULL);
    }
#endif

  if (self->splash)
    {
      splash_window_widget_close (self->splash);
      self->splash = NULL;
    }

  g_message ("done");
}

/**
 * Called after the main window has been
 * initialized.
 *
 * Loads the project backend or creates the default
 * one.
 * FIXME rename
 */
static void on_load_project (
  GSimpleAction  *action,
  GVariant *parameter,
  gpointer  user_data)
{
  zrythm_app_set_progress_status (
    zrythm_app,
    _("Loading project"),
    0.8);

  int ret =
    project_load (
      ZRYTHM->open_filename,
      ZRYTHM->opening_template);

  if (ret != 0)
    {
      char msg[600];
      sprintf (
        msg,
        _("No project has been selected. %s "
        "will now close."),
        PROGRAM_NAME);
      ui_show_error_message (
        NULL, msg);
      exit (0);
    }

  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app),
    "setup_main_window",
    NULL);
}

static void *
init_thread (
  gpointer data)
{
  g_message ("init thread starting...");

  ZrythmApp * self = ZRYTHM_APP (data);

  zrythm_app_set_progress_status (
    self, _("Initializing settings"), 0.0);

  ZRYTHM->debug =
    env_get_int ("ZRYTHM_DEBUG", 0);
  /* init zrythm folders ~/Zrythm */
  char msg[500];
  sprintf (
    msg,
    _("Initializing %s directories"),
    PROGRAM_NAME);
  zrythm_app_set_progress_status (
    self, msg, 0.01);
  init_dirs_and_files ();
  init_recent_projects ();
  init_templates ();

  /* init log */
  zrythm_app_set_progress_status (
    self, _("Initializing logging system"), 0.02);
  log_init_with_file (LOG, false, -1);

#if defined (_WOE32) || defined (__APPLE__)
  g_warning (
    "Warning, you are running a non-free operating "
    "system.");
#endif

  zrythm_app_set_progress_status (
    self, _("Initializing caches"), 0.05);
  self->ui_caches = ui_caches_new ();

  zrythm_app_set_progress_status (
    self, _("Initializing file manager"), 0.15);
  file_manager_load_files (FILE_MANAGER);

  if (!g_settings_get_boolean (
         S_GENERAL, "first-run"))
    {
      zrythm_app_set_progress_status (
        self, _("Scanning plugins"), 0.4);
      plugin_manager_scan_plugins (
        ZRYTHM->plugin_manager, 0.7,
        &ZRYTHM->progress);
    }

  self->init_finished = true;

  g_message ("done");

  return NULL;
}

/**
 * Unlike the init thread, this will run in the
 * main GTK thread. Do not put expensive logic here.
 *
 * This should be ran after the expensive
 * initialization has finished.
 */
static int
idle_func (
  ZrythmApp * self)
{
  if (self->init_finished)
    {
      log_init_writer_idle (ZRYTHM->log, 3);

      g_action_group_activate_action (
        G_ACTION_GROUP (self),
        "prompt_for_project", NULL);

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Thread that scans for plugins after the first
 * run.
 */
static void *
scan_plugins_after_first_run_thread (
  gpointer data)
{
  g_message ("scanning...");

  ZrythmApp * self = ZRYTHM_APP (data);

  plugin_manager_scan_plugins (
    ZRYTHM->plugin_manager,
    0.7, &ZRYTHM->progress);

  self->init_finished = true;

  g_message ("done");

  return NULL;
}

static void
on_first_run_assistant_apply (
  GtkAssistant * _assistant,
  ZrythmApp *    self)
{
  g_message ("on apply...");

  g_settings_set_boolean (
    S_GENERAL, "first-run", 0);

  /* start plugin scanning in another thread */
  self->init_thread =
    g_thread_new (
      "scan_plugins_after_first_run_thread",
      (GThreadFunc)
      scan_plugins_after_first_run_thread,
      self);

  /* set a source func in the main GTK thread to
   * check when scanning finished */
  self->init_finished = 0;
  g_idle_add ((GSourceFunc) idle_func, self);

  gtk_widget_set_visible (
    GTK_WIDGET (self->first_run_assistant), 0);

  /* close the first run assistant if it ran
   * before */
  if (self->assistant)
    {
      DESTROY_LATER (_assistant);
      self->first_run_assistant = NULL;
    }

  g_message ("done");
}

static void
on_first_run_assistant_cancel ()
{
  g_message ("%s: cancel", __func__);

  exit (0);
}

/**
 * Called before on_load_project.
 *
 * Checks if a project was given in the command
 * line. If not, it prompts the user for a project
 * (start assistant).
 */
static void on_prompt_for_project (
  GSimpleAction * action,
  GVariant *      parameter,
  gpointer        data)
{
  g_message ("prompting...");

  ZrythmApp * self = zrythm_app;

  if (ZRYTHM->open_filename)
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (self),
        "load_project", NULL);
    }
  else
    {
      if (g_settings_get_boolean (
            S_GENERAL, "first-run"))
        {
          /* warranty disclaimer */
          GtkDialogFlags flags =
            GTK_DIALOG_DESTROY_WITH_PARENT;
          GtkWidget * dialog =
            gtk_message_dialog_new (
              NULL, flags,
              GTK_MESSAGE_INFO,
              GTK_BUTTONS_OK,
"Copyright (C) 2018-2020 The Zrythm contributors\n"
"\n"
"Zrythm is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU Affero General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"Zrythm is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU Affero General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU Affero General Public License\n"
"along with Zrythm.  If not, see <https://www.gnu.org/licenses/>."
#if !defined(HAVE_CUSTOM_NAME) || !defined(HAVE_CUSTOM_LOGO_AND_SPLASH)
"\n\nZrythm and the Zrythm logo are trademarks of Alexandros Theodotou"
#endif
);
          gtk_window_set_title (
            GTK_WINDOW (dialog),
            _("License Information"));
          gtk_window_set_icon_name (
            GTK_WINDOW (dialog), "zrythm");
          gtk_dialog_run (GTK_DIALOG (dialog));
          gtk_widget_destroy (dialog);

          self->first_run_assistant =
            first_run_assistant_widget_new (
              GTK_WINDOW (self->splash));
          g_signal_connect (
            G_OBJECT (self->first_run_assistant),
            "apply",
            G_CALLBACK (
              on_first_run_assistant_apply), self);
          g_signal_connect (
            G_OBJECT (self->first_run_assistant),
            "cancel",
            G_CALLBACK (
              on_first_run_assistant_cancel),
            NULL);
          gtk_window_present (
            GTK_WINDOW (
              self->first_run_assistant));

          return;
        }

      zrythm_app_set_progress_status (
        self, _("Waiting for project"), 0.8);

      /* show the assistant */
      self->assistant =
        project_assistant_widget_new (
          GTK_WINDOW (self->splash), 1);
      gtk_widget_set_visible (
        GTK_WIDGET (self->assistant), 1);

#ifdef __APPLE__
  /* possibly not necessary / working, forces app *
   * window on top */
  show_on_top();
#endif
    }

  g_message ("done");
}

/**
 * Sets the current status and progress percentage
 * during loading.
 *
 * The splash screen then reads these values from
 * the Zrythm struct.
 */
void
zrythm_app_set_progress_status (
  ZrythmApp *  self,
  const char * text,
  const double perc)
{
  zix_sem_wait (&self->progress_status_lock);
  g_message ("%s", text);
  strcpy (self->status, text);
  ZRYTHM->progress = perc;
  zix_sem_post (&self->progress_status_lock);
}

/*
 * Called after startup if no filename is passed on
 * command line.
 */
static void
zrythm_app_activate (
  GApplication * _app)
{
  g_message ("Activating...");
  /*g_message ("activate %d", *task_id);*/

  /* init localization, using system locale if
   * first run */
  GSettings * prefs =
    g_settings_new (
      GSETTINGS_ZRYTHM_PREFIX ".general");
  localization_init (
    g_settings_get_boolean (
      prefs, "first-run"), true);
  g_object_unref (G_OBJECT (prefs));

  g_message ("done");
}

/**
 * Called when a filename is passed to the command line
 * instead of activate.
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
  g_message ("Opening...");

  g_warn_if_fail (n_files == 1);

  GFile * file = files[0];
  ZRYTHM->open_filename =
    g_file_get_path (file);
  g_message ("open %s", ZRYTHM->open_filename);

  g_message ("done");
}

static void
print_gdk_pixbuf_format_info (
  gpointer data,
  gpointer user_data)
{
  GdkPixbufFormat * format =
    (GdkPixbufFormat *) data;
  ZrythmApp * self = ZRYTHM_APP (user_data);

  char * name =
    gdk_pixbuf_format_get_name (format);
  char * description =
    gdk_pixbuf_format_get_description (format);
  char * license =
    gdk_pixbuf_format_get_license (format);
  char mime_types[800] = "";
  char ** _mime_types =
    gdk_pixbuf_format_get_mime_types (
      format);
  char * tmp = NULL;
  int i = 0;
  while ((tmp = _mime_types[i++]))
    {
      strcat (mime_types, tmp);
      strcat (mime_types, ", ");
    }
  mime_types[strlen (mime_types) - 2] = '\0';
  g_strfreev (_mime_types);
  char extensions[800] = "";
  char ** _extensions =
    gdk_pixbuf_format_get_extensions (
      format);
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
  g_message (
    "Found GDK Pixbuf Format:\n"
    "name: %s\ndescription: %s\n"
    "mime types: %s\nextensions: %s\n"
    "is scalable: %d\nis disabled: %d\n"
    "license: %s",
    name, description, mime_types,
    extensions,
    gdk_pixbuf_format_is_scalable (format),
    gdk_pixbuf_format_is_disabled (format),
    license);
  g_free (name);
  g_free (description);
  g_free (license);
}

static void
load_icon (
  GtkIconTheme * icon_theme,
  const char *   icon_name)
{
  g_message (
    "Attempting to load an icon from the icon "
    "theme...");
  GError * err = NULL;
  GdkPixbuf * icon =
    gtk_icon_theme_load_icon (
      icon_theme, icon_name, 48, 0, &err);
  g_message ("icon: %p", icon);
  if (err)
    {
      char err_msg[600];
      sprintf (
        err_msg,
        "Failed to load icon from icon theme: %s. "
        "Please install zrythm and breeze-icons.",
        err->message);
      g_critical ("%s", err_msg);
      fprintf (stderr, "%s\n", err_msg);
      ui_show_message_full (
        NULL, GTK_MESSAGE_ERROR, err_msg);
      g_error ("Failed to load icon");
    }
  g_object_unref (icon);
  g_message ("Icon loaded.");
}

/**
 * First function that gets called.
 */
static void
zrythm_app_startup (
  GApplication * app)
{
  g_message ("Starting up...");

  ZrythmApp * self = ZRYTHM_APP (app);
  G_APPLICATION_CLASS (zrythm_app_parent_class)->
    startup (G_APPLICATION (self));
  g_message (
    "called startup on G_APPLICATION_CLASS");

  self->default_settings =
    gtk_settings_get_default ();

  /* set theme */
  g_object_set (
    self->default_settings,
    "gtk-theme-name", "Matcha-dark-sea", NULL);
  g_object_set (
    self->default_settings,
    "gtk-application-prefer-dark-theme", 1, NULL);
  GdkDisplay * display =
    gdk_display_get_default ();
  g_warn_if_fail (display);
  GdkMonitor * monitor =
    gdk_display_get_primary_monitor (display);
  g_warn_if_fail (monitor);
  int scale_factor =
    gdk_monitor_get_scale_factor (monitor);
  g_message (
    "Monitor scale factor: %d", scale_factor);
#if defined(_WOE32)
  g_object_set (
    self->default_settings,
    "gtk-font-name", "Segoe UI Normal 10", NULL);
  g_object_set (
    self->default_settings,
    "gtk-cursor-theme-name", "Adwaita", NULL);
  /*g_object_set (*/
    /*self->default_settings,*/
    /*"gtk-icon-theme-name", "breeze-dark", NULL);*/
#elif defined(__APPLE__)
  g_object_set (
    self->default_settings,
    "gtk-font-name", "Regular 10", NULL);
  /* explicitly set font scaling to 1.00 (for some
   * reason, a different value is used by default).
   * this was taken from the GtkInspector visual.c
   * code */
  /* TODO add an option to change the font scaling */
  g_object_set (
    self->default_settings,
    "gtk-xft-dpi", (int) (1.00 * 96 * 1024), NULL);
#else
  g_object_set (
    self->default_settings,
    "gtk-font-name", "Cantarell Regular 10", NULL);
#endif
  g_message ("Theme set");

  GtkIconTheme * icon_theme =
    gtk_icon_theme_get_default ();
  g_object_set (
    self->default_settings, "gtk-icon-theme-name",
    "zrythm-dark", NULL);

  /* prepend freedesktop system icons to search
   * path, just in case */
  char * parent_datadir =
    zrythm_get_dir (ZRYTHM_DIR_SYSTEM_PARENT_DATADIR);
  char * freedesktop_icon_theme_dir =
    g_build_filename (
      parent_datadir, "icons", NULL);
  gtk_icon_theme_prepend_search_path (
    icon_theme, freedesktop_icon_theme_dir);
  g_message (
    "prepended icon theme search path: %s",
    freedesktop_icon_theme_dir);
  g_free (parent_datadir);
  g_free (freedesktop_icon_theme_dir);

  /* prepend zrythm system icons to search path */
  char * system_themes_dir =
    zrythm_get_dir (ZRYTHM_DIR_SYSTEM_THEMESDIR);
  char * system_icon_theme_dir =
    g_build_filename (
      system_themes_dir, "icons", NULL);
  gtk_icon_theme_prepend_search_path (
    icon_theme,
    system_icon_theme_dir);
  g_message (
    "prepended icon theme search path: %s",
    system_icon_theme_dir);
  g_free (system_themes_dir);
  g_free (system_icon_theme_dir);

  /* prepend user custom icons to search path */
  char * user_themes_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_THEMES);
  char * user_icon_theme_dir =
    g_build_filename (
      user_themes_dir, "icons", NULL);
  gtk_icon_theme_prepend_search_path (
    icon_theme,
    user_icon_theme_dir);
  g_message (
    "prepended icon theme search path: %s",
    user_icon_theme_dir);
  g_free (user_themes_dir);
  g_free (user_icon_theme_dir);

  /* look for found loaders */
  g_message ("looking for GDK Pixbuf formats...");
  GSList * formats_list =
    gdk_pixbuf_get_formats ();
  g_slist_foreach (
    formats_list, print_gdk_pixbuf_format_info,
    self);
  g_slist_free (g_steal_pointer (&formats_list));
  if (!self->have_svg_loader)
    {
      fprintf (
        stderr, "SVG loader was not found.\n");
      exit (-1);
    }

  /* try to load some icons */
  /* zrythm */
  load_icon (icon_theme, "solo");
  /* breeze dark */
  load_icon (icon_theme, "node-type-cusp");

  g_message ("Setting gtk icon theme resource paths...");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/gnome-builder");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/breeze-icons");

  /*gtk_icon_theme_set_search_path (*/
    /*gtk_icon_theme_get_default (),*/
    /*path,*/
    /*1);*/
  g_message ("Resource paths set");

  /* set default css provider */
  GtkCssProvider * css_provider =
    gtk_css_provider_new();
  user_themes_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_THEMES);
  char * css_theme_path =
    g_build_filename (
      user_themes_dir, "theme.css", NULL);
  g_free (user_themes_dir);
  if (!g_file_test (
         css_theme_path, G_FILE_TEST_EXISTS))
    {
      g_free (css_theme_path);
      system_themes_dir =
        zrythm_get_dir (
          ZRYTHM_DIR_SYSTEM_THEMESDIR);
      css_theme_path =
        g_build_filename (
          system_themes_dir,
          "zrythm-theme.css", NULL);
      g_free (system_themes_dir);
    }
  GError * err = NULL;
  gtk_css_provider_load_from_path (
    css_provider, css_theme_path, &err);
  if (err)
    {
      g_warning (
        "Failed to load CSS from path %s: %s",
        css_theme_path, err->message);
    }
  gtk_style_context_add_provider_for_screen (
    gdk_screen_get_default (),
    GTK_STYLE_PROVIDER (css_provider), 800);
  g_object_unref (css_provider);
  g_message (
    "set default css provider from path: %s",
    css_theme_path);

  /* set default window icon */
  gtk_window_set_default_icon_name ("zrythm");

  /* show splash screen */
  self->splash =
    splash_window_widget_new (self);
  g_message ("created splash widget");
  gtk_window_present (
    GTK_WINDOW (self->splash));
  g_message ("presented splash widget");

  /* start initialization in another thread */
  zix_sem_init (&self->progress_status_lock, 1);
  self->init_thread =
    g_thread_new (
      "init_thread", (GThreadFunc) init_thread,
      self);

  /* set a source func in the main GTK thread to
   * check when initialization finished */
  g_idle_add ((GSourceFunc) idle_func, self);

  /* install accelerators for each action */
#define INSTALL_ACCEL(keybind,action) \
  accel_install_primary_action_accelerator ( \
    keybind, action)

  INSTALL_ACCEL ("<Alt>F4", "app.quit");
  INSTALL_ACCEL ("F11", "app.fullscreen");
  INSTALL_ACCEL (
    "<Control><Shift>p", "app.preferences");
  INSTALL_ACCEL (
    "<Control><Shift>question", "app.shortcuts");
  INSTALL_ACCEL ("<Control>n", "win.new");
  INSTALL_ACCEL ("<Control>o", "win.open");
  INSTALL_ACCEL ("<Control>s", "win.save");
  INSTALL_ACCEL (
    "<Control><Shift>s", "win.save-as");
  INSTALL_ACCEL ("<Control>e", "win.export-as");
  INSTALL_ACCEL ("<Control>z", "win.undo");
  INSTALL_ACCEL (
    "<Control><Shift>z", "win.redo");
  INSTALL_ACCEL (
    "<Control>x", "win.cut");
  INSTALL_ACCEL (
    "<Control>c", "win.copy");
  INSTALL_ACCEL (
    "<Control>v", "win.paste");
  INSTALL_ACCEL (
    "<Control>d", "win.duplicate");
  INSTALL_ACCEL (
    "Delete", "win.delete");
  INSTALL_ACCEL (
    "<Control>backslash", "win.clear-selection");
  INSTALL_ACCEL (
    "<Control>a", "win.select-all");
  INSTALL_ACCEL (
    "<Control><Shift>4", "win.toggle-left-panel");
  INSTALL_ACCEL (
    "<Control><Shift>6", "win.toggle-right-panel");
  INSTALL_ACCEL (
    "<Control><Shift>2", "win.toggle-bot-panel");
  INSTALL_ACCEL (
    "<Control>equal", "win.zoom-in");
  INSTALL_ACCEL (
    "<Control>minus", "win.zoom-out");
  INSTALL_ACCEL (
    "<Control>plus", "win.original-size");
  INSTALL_ACCEL (
    "<Control>bracketleft", "win.best-fit");
  INSTALL_ACCEL (
    "<Control>l", "win.loop-selection");
  INSTALL_ACCEL (
    "1", "win.select-mode");
  INSTALL_ACCEL (
    "2", "win.edit-mode");
  INSTALL_ACCEL (
    "3", "win.cut-mode");
  INSTALL_ACCEL (
    "4", "win.eraser-mode");
  INSTALL_ACCEL (
    "5", "win.ramp-mode");
  INSTALL_ACCEL (
    "6", "win.audition-mode");
  accel_install_action_accelerator (
    "KP_4", "BackSpace", "win.goto-prev-marker");
  INSTALL_ACCEL (
    "KP_6", "win.goto-next-marker");
  INSTALL_ACCEL (
    "space", "win.play-pause");
  INSTALL_ACCEL (
    "Q", "win.quick-quantize::global");
  INSTALL_ACCEL (
    "<Alt>Q", "win.quantize-options::global");
  INSTALL_ACCEL (
    "<Shift>M", "win.mute-selection::global");

#undef INSTALL_ACCEL

  g_message ("done");
}

static void
lock_memory (void)
{
#ifdef _WOE32
  /* TODO */
#else
  /* lock down memory */
  g_message ("Locking down memory...");
  if (mlockall (MCL_CURRENT))
    {
      g_warning ("Cannot lock down memory: %s",
                 strerror (errno));
    }
#endif
}

/**
 * Called immediately after the main GTK loop
 * terminates.
 *
 * This is also called manually on SIGINT.
 */
static void
zrythm_app_on_shutdown (
  GApplication * application,
  ZrythmApp *    self)
{
  g_message ("Shutting down...");

  if (ZRYTHM)
    {
      zrythm_free (ZRYTHM);
    }

#ifdef HAVE_GTK_SOURCE_VIEW_4
  gtk_source_finalize ();
#endif

  g_message ("done");
}

/**
 * Creates the Zrythm GApplication.
 *
 * This also initializes the Zrythm struct.
 */
ZrythmApp *
zrythm_app_new (
  char * audio_backend,
  char * midi_backend,
  char * buf_size)
{
  ZrythmApp * self =  g_object_new (
    ZRYTHM_APP_TYPE,
    /* if an ID is provided, this application
     * becomes unique (only one instance allowed) */
    /*"application-id", "org.zrythm.Zrythm",*/
    "resource-base-path", "/org/zrythm/Zrythm",
    "flags", G_APPLICATION_HANDLES_OPEN,
    NULL);

  self->gtk_thread = g_thread_self ();

  self->audio_backend = audio_backend;
  self->midi_backend = midi_backend;
  self->buf_size = buf_size;

  lock_memory ();
  ZRYTHM = zrythm_new (true, false);

  /* add shutdown handler */
  g_signal_connect (
    G_OBJECT (self), "shutdown",
    G_CALLBACK (zrythm_app_on_shutdown), self);

  return self;
}

static void
finalize (
  ZrythmApp * self)
{
  g_message (
    "%s (%s): finalizing ZrythmApp...",
    __func__, __FILE__);

  g_object_unref (self->default_settings);

  object_free_w_func_and_null (
    ui_caches_free, self->ui_caches);

  g_message ("%s: done", __func__);
}

static void
zrythm_app_class_init (ZrythmAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate =
    zrythm_app_activate;
  G_APPLICATION_CLASS (class)->startup =
    zrythm_app_startup;
  G_APPLICATION_CLASS (class)->open =
    zrythm_app_open;
  /*G_APPLICATION_CLASS (class)->shutdown =*/
    /*zrythm_app_shutdown;*/

  GObjectClass * klass = G_OBJECT_CLASS (class);

  klass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
zrythm_app_init (ZrythmApp * _app)
{
  g_message ("initing zrythm app");

  /* prefer x11 backend because plugin UIs need
   * it to load */
  gdk_set_allowed_backends ("quartz,win32,x11,*");

  const GActionEntry entries[] = {
    { "prompt_for_project", on_prompt_for_project },
    /*{ "init_main_window", on_init_main_window },*/
    { "setup_main_window", on_setup_main_window },
    { "load_project", on_load_project },
    { "about", activate_about },
    { "fullscreen", activate_fullscreen },
    { "chat", activate_chat },
    { "manual", activate_manual },
    { "news", activate_news },
    { "bugreport", activate_bugreport },
    { "donate", activate_donate },
    { "iconify", activate_iconify },
    { "log", activate_log },
    { "preferences", activate_preferences },
    { "scripting-interface",
      activate_scripting_interface },
    { "quit", activate_quit },
    { "shortcuts", activate_shortcuts },
  };

  g_action_map_add_action_entries (
    G_ACTION_MAP (_app),
    entries,
    G_N_ELEMENTS (entries),
    _app);

  g_message ("added action entries");
}
