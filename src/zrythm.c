/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#include <stdlib.h>

#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/quantize_options.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/about_dialog.h"
#include "gui/widgets/create_project_dialog.h"
#include "gui/widgets/first_run_assistant.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/project_assistant.h"
#include "gui/widgets/splash.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2/symap.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/io.h"
#include "zrythm.h"

#include "Wrapper.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (ZrythmApp,
               zrythm_app,
               GTK_TYPE_APPLICATION);

typedef struct UpdateSplashData
{
  const char * message;
  gdouble progress;
} UpdateSplashData;

typedef enum TaskId
{
  TASK_START,
  TASK_INIT_SETTINGS,
  TASK_INIT_AUDIO_ENGINE,
  TASK_INIT_PLUGIN_MANAGER,
  TASK_END
} TaskId;

static SplashWindowWidget * splash;
static GApplication * app;
static FirstRunAssistantWidget * first_run_assistant;
static ProjectAssistantWidget * assistant;
static TaskId * task_id; ///< current task;
static UpdateSplashData * data;

static int
update_splash (
  UpdateSplashData * _data)
{
  /* sometimes this gets called after the splash window
   * gets deleted so added for safety */
  if (!first_run_assistant &&
      splash && GTK_IS_WIDGET (splash))
    {
      splash_widget_update (
        splash, data->message, data->progress);
    }
  return G_SOURCE_REMOVE;
}

/**
 * Gets the zrythm directory (by default
 * /home/user/zrythm).
 *
 * Must be free'd by caler.
 */
char *
zrythm_get_dir (
  Zrythm * self)
{
  g_warn_if_fail (S_PREFERENCES != NULL);

  char * dir =
    g_settings_get_string (
      S_GENERAL,
      "dir");
  return dir;
}

/**
 * Initializes/creates the default dirs/files.
 */
static void
init_dirs_and_files ()
{
  g_message ("initing dirs and files");
  ZRYTHM->zrythm_dir =
    zrythm_get_dir (ZRYTHM);
  io_mkdir (ZRYTHM->zrythm_dir);

  ZRYTHM->projects_dir =
    g_build_filename (ZRYTHM->zrythm_dir,
                      "Projects",
                      NULL);
  io_mkdir (ZRYTHM->projects_dir);

  ZRYTHM->templates_dir =
    g_build_filename (ZRYTHM->zrythm_dir,
                      "Templates",
                      NULL);
  io_mkdir (ZRYTHM->templates_dir);

  ZRYTHM->log_dir =
    g_build_filename (ZRYTHM->zrythm_dir,
                      "log",
                      NULL);
  io_mkdir (ZRYTHM->log_dir);
}

/**
 * Initializes the array of recent projects in
 * Zrythm app.
 */
static void
init_recent_projects ()
{
  gchar ** recent_projects =
    g_settings_get_strv (S_GENERAL,
                         "recent-projects");

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
    S_GENERAL,
    "recent-projects",
    (const char * const *) ZRYTHM->recent_projects);
}

/**
 * Initializes the array of recent projects in
 * Zrythm app.
 */
static void
init_templates ()
{

  ZRYTHM->templates =
    io_get_files_in_dir (ZRYTHM->templates_dir);
}

/**
 * FIXME move somewhere else.
 */
void
zrythm_add_to_recent_projects (
  Zrythm * self,
  const char * _filepath)
{
  /* if we are at max
   * projects */
  if (ZRYTHM->num_recent_projects ==
        MAX_RECENT_PROJECTS)
    {
      /* free the last one and delete it */
      g_free (ZRYTHM->recent_projects[
                MAX_RECENT_PROJECTS - 1]);
      array_delete (
        ZRYTHM->recent_projects,
        ZRYTHM->num_recent_projects,
        ZRYTHM->recent_projects[
          ZRYTHM->num_recent_projects - 1]);
    }

  char * filepath =
    g_strdup (_filepath);

  array_insert (
    ZRYTHM->recent_projects,
    ZRYTHM->num_recent_projects,
    0,
    filepath);

  /* set last element to NULL because the call
   * takes a NULL terminated array */
  ZRYTHM->recent_projects[
    ZRYTHM->num_recent_projects] = NULL;

  g_settings_set_strv (
    S_GENERAL,
    "recent-projects",
    (const char * const *) ZRYTHM->recent_projects);
}

void
zrythm_remove_recent_project (
  char * filepath)
{
  for (int i = 0; i < ZRYTHM->num_recent_projects;
       i++)
    {
      if (!strcmp (filepath,
                   ZRYTHM->recent_projects[i]))
        {
          array_delete (ZRYTHM->recent_projects,
                        ZRYTHM->num_recent_projects,
                        ZRYTHM->recent_projects[i]);

          ZRYTHM->recent_projects[
            ZRYTHM->num_recent_projects] = NULL;

          g_settings_set_strv (
            S_GENERAL,
            "recent-projects",
            (const char * const *)
              ZRYTHM->recent_projects);
        }

    }

}

static void
task_func (
  GTask *task,
  gpointer source_object,
  gpointer task_data,
  GCancellable *cancellable)
{
  /* sleep to give the splash screen time to open.
   * for some reason it delays if no sleep */
  g_usleep (10000);

  switch (*task_id)
    {
    case TASK_START:
      data->message =
        _("Initializing settings");
      data->progress = 0.3;
      break;
    case TASK_INIT_SETTINGS:
      ZRYTHM->symap = symap_new ();
      settings_init (&ZRYTHM->settings);
      ZRYTHM->debug =
        g_settings_get_int (
          S_GENERAL,
          "debug");
      data->message =
        _("Initializing audio engine");
      data->progress = 0.4;
      break;
    case TASK_INIT_AUDIO_ENGINE:
      data->message =
        _("Initializing plugin manager");
      data->progress = 0.6;
      break;
    case TASK_INIT_PLUGIN_MANAGER:
      plugin_manager_init (&ZRYTHM->plugin_manager);
      data->message =
        _("Setting up backend");
      data->progress = 0.7;
      break;
    case TASK_END:
      plugin_manager_scan_plugins (
        &ZRYTHM->plugin_manager);
      data->message =
        _("Loading project");
      data->progress = 0.8;
      break;
    }
}

static void
task_completed_cb (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
  g_idle_add ((GSourceFunc) update_splash, data);
  if (*task_id == TASK_END)
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (zrythm_app),
        "prompt_for_project",
        NULL);
    }
  else
    {
      (*task_id)++;
      GTask * task = g_task_new (zrythm_app,
                                 NULL,
                                 task_completed_cb,
                                 (gpointer) task_id);
      g_task_set_task_data (task,
                            (gpointer) task_id,
                            NULL);
      g_task_run_in_thread (task, task_func);
    }
}

/**
 * Called after the main window and the project have been
 * initialized. Sets up the window using the backend.
 *
 * This is the final step.
 */
static void
on_setup_main_window (
  GSimpleAction  *action,
  GVariant *parameter,
  gpointer  user_data)
{
  g_message ("setup main window");

  ZRYTHM->event_queue =
    events_init ();
  main_window_widget_refresh (MAIN_WINDOW);

  mixer_recalc_graph (MIXER);
  g_atomic_int_set (&AUDIO_ENGINE->run, 1);

  /* add timeout for auto-saving projects */
  unsigned int autosave_interval =
    g_settings_get_uint (
      S_PREFERENCES, "autosave-interval") * 60;
  if (autosave_interval > 0)
    {
      g_timeout_add_seconds (
        autosave_interval,
        project_autosave_cb,
        NULL);
    }

  free (data);
}

/**
 * Called after the main window has been initialized.
 *
 * Loads the project backend or creates the default
 * one.
 * FIXME rename
 */
static void on_load_project (GSimpleAction  *action,
                             GVariant *parameter,
                             gpointer  user_data)
{
  g_message ("load_project");

  project_load (
    ZRYTHM->open_filename,
    ZRYTHM->opening_template);

  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app),
    "setup_main_window",
    NULL);
}

/**
 * Called after the user made a choice for a project or gave
 * the project filename in the command line.
 *
 * It initializes the main window and shows it (not set up
 * yet)
 */
static void on_init_main_window (
  GSimpleAction  *action,
  GVariant *parameter,
  void *          user_data)
{
  ZrythmApp * _app = (ZrythmApp *) user_data;
  g_message ("init main window");

  gtk_widget_destroy (GTK_WIDGET (splash));

  ZRYTHM->main_window =
    main_window_widget_new (_app);

  g_action_group_activate_action (
    G_ACTION_GROUP (_app),
    "load_project",
    NULL);
}

static void
on_finish (GtkAssistant * _assistant,
          gpointer       user_data)
{
  zrythm->creating_project = 1;
  if (user_data) /* if cancel */
    {
      gtk_widget_destroy (GTK_WIDGET (assistant));
      zrythm->open_filename = NULL;
    }
  /* if we are loading a template and template
   * exists */
  else if (assistant->load_template &&
           assistant->template_selection &&
           assistant->template_selection->
             filename[0] != '-')
    {
      zrythm->open_filename =
        assistant->template_selection->filename;
      g_message (
        "Creating project from template: %s",
        zrythm->open_filename);
      zrythm->opening_template = 1;
    }
  /* if we are loading a project */
  else if (!assistant->load_template &&
           assistant->project_selection)
    {
      zrythm->open_filename =
        assistant->project_selection->filename;
      g_message (
        "Loading project: %s",
        zrythm->open_filename);
      zrythm->creating_project = 0;
    }
  /* no selection, load blank project */
  else
    {
      zrythm->open_filename = NULL;
      g_message (
        "Creating blank project");
    }

  /* if not loading a project, show dialog to
   * select directory and name */
  int quit = 0;
  if (zrythm->creating_project)
    {
      CreateProjectDialogWidget * dialog =
        create_project_dialog_widget_new ();

      int ret =
        gtk_dialog_run (GTK_DIALOG (dialog));
      if (ret != GTK_RESPONSE_OK)
        quit = 1;
      gtk_widget_destroy (GTK_WIDGET (dialog));

      g_message ("creating project %s",
                 zrythm->create_project_path);
    }

  if (quit)
    {
      g_application_quit (
        G_APPLICATION (zrythm_app));
    }
  else
    {
      gtk_widget_set_visible (
        GTK_WIDGET (assistant), 0);
      data->message =
        "Finishing";
      data->progress = 1.0;
      update_splash (data);
      g_action_group_activate_action (
        G_ACTION_GROUP (zrythm_app),
        "init_main_window",
        NULL);
    }
}

static void
on_first_run_assistant_apply (
  GtkAssistant * _assistant)
{
  g_message ("apply");

  g_settings_set_int (
    S_GENERAL, "first-run", 0);

  g_action_group_activate_action (
  G_ACTION_GROUP (zrythm_app),
  "prompt_for_project",
  NULL);

  /* close the first run assistant if it ran
   * before */
  if (assistant)
    {
      DESTROY_LATER (_assistant);
      first_run_assistant = NULL;
    }
}

static void
on_first_run_assistant_cancel ()
{
  g_message ("cancel");

  exit (0);
}

/**
 * Called before on_load_project.
 *
 * Checks if a project was given in the command line. If not,
 * it prompts the user for a project (start assistant).
 */
static void on_prompt_for_project (
  GSimpleAction * action,
  GVariant *      parameter,
  void *          user_data)
{
  ZrythmApp * _app = (ZrythmApp *) user_data;
  g_message ("prompt for project");

  if (ZRYTHM->open_filename)
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (_app),
        "init_main_window",
        NULL);
    }
  else
    {
      if (g_settings_get_int (
            S_GENERAL,
            "first-run"))
        {
          /* warranty disclaimer */
          GtkDialogFlags flags =
            GTK_DIALOG_DESTROY_WITH_PARENT;
          GtkWidget * dialog =
            gtk_message_dialog_new (
              NULL,
              flags,
              GTK_MESSAGE_INFO,
              GTK_BUTTONS_OK,
"Copyright (C) 2018-2019 Alexandros Theodotou\n"
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
"along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.");
          gtk_window_set_title (
            GTK_WINDOW (dialog),
            _("License Information"));
          gtk_dialog_run (GTK_DIALOG (dialog));
          gtk_widget_destroy (dialog);

          first_run_assistant =
            first_run_assistant_widget_new (
              GTK_WINDOW (splash));
          g_signal_connect (
            G_OBJECT (first_run_assistant), "apply",
            G_CALLBACK (on_first_run_assistant_apply), NULL);
          g_signal_connect (
            G_OBJECT (first_run_assistant), "cancel",
            G_CALLBACK (on_first_run_assistant_cancel), NULL);
          gtk_window_present (GTK_WINDOW (first_run_assistant));

          return;
        }

      /* init zrythm folders ~/Zrythm */
      init_dirs_and_files ();
      init_recent_projects ();
      init_templates ();

      /* init log */
      g_message ("Initing log...");
      log_init ();

      /* show the assistant */
      assistant =
        project_assistant_widget_new (
          GTK_WINDOW(splash), 1);
      g_signal_connect (
        G_OBJECT (assistant), "apply",
        G_CALLBACK (on_finish), NULL);
      g_signal_connect (
        G_OBJECT (assistant), "cancel",
        G_CALLBACK (on_finish), (void *) 1);
      /*g_idle_add (*/
        /*(GSourceFunc) gtk_window_present,*/
        /*GTK_WINDOW (assistant));*/
      gtk_window_present (GTK_WINDOW (assistant));

#ifdef __APPLE__
  /* possibly not necessary / working, forces app *
   * window on top */
  show_on_top();
#endif
    }
}

/**
 * Returns the version string.
 *
 * Must be g_free()'d.
 *
 * @param with_v Include a starting "v".
 */
char *
zrythm_get_version (
  int with_v)
{
  const char * ver = PACKAGE_VERSION;

  if (with_v)
    {
      if (ver[0] == 'v')
        return g_strdup (ver);
      else
        return
          g_strdup_printf ("v%s", ver);
    }
  else
    {
      if (ver[0] == 'v')
        return g_strdup (ver + 1);
      else
        return g_strdup (ver);
    }
}

/*
 * Called after startup if no filename is passed on command
 * line.
 */
static void
zrythm_app_activate (GApplication * _app)
{
  /*g_message ("activate %d", *task_id);*/

  /* init localization */
  localization_init ();
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
  g_warn_if_fail (n_files == 1);

  GFile * file = files[0];
  zrythm->open_filename = g_file_get_path (file);
  g_message ("open %s", zrythm->open_filename);
}

/**
 * First function that gets called.
 */
static void
zrythm_app_startup (
  GApplication* _app)
{
  g_message ("startup");
  G_APPLICATION_CLASS (zrythm_app_parent_class)->
    startup (_app);
  g_message ("called startup on G_APPLICATION_CLASS");

  app = _app;

  /* set theme */
  g_object_set (gtk_settings_get_default (),
                "gtk-theme-name",
                "Matcha-dark-sea",
                NULL);
  g_message ("set theme");

  /*g_object_set (gtk_settings_get_default (),*/
                /*"gtk-icon-theme-name",*/
                /*"breeze-dark",*/
                /*NULL);*/

  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/breeze-icons");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/gnome-builder");

  /*gtk_icon_theme_set_search_path (*/
    /*gtk_icon_theme_get_default (),*/
    /*path,*/
    /*1);*/
  g_message ("set resource paths");

  // set default css provider
  GtkCssProvider * css_provider =
    gtk_css_provider_new();
  gtk_css_provider_load_from_resource (
    css_provider,
    "/org/zrythm/Zrythm/app/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);
  g_message ("set default css provider");

  /* show splash screen */
  splash =
    splash_window_widget_new (ZRYTHM_APP (app));
  g_message ("created splash widget");
  gtk_window_present (
    GTK_WINDOW (splash));
  g_message ("presented splash widget");

  /* start initialization task */
  task_id = calloc (1, sizeof (TaskId));
  *task_id = TASK_START;
  data = calloc (1, sizeof (UpdateSplashData));
  GTask * task = g_task_new (app,
                             NULL,
                             task_completed_cb,
                             (gpointer) task_id);
  g_task_set_task_data (task,
                        (gpointer) task_id,
                        NULL);
  g_task_run_in_thread (task, task_func);

  /* install accelerators for each action */
  accel_install_primary_action_accelerator (
    "<Alt>F4",
    "app.quit");
  accel_install_primary_action_accelerator (
    "F11",
    "app.fullscreen");
  accel_install_primary_action_accelerator (
    "<Control><Shift>p",
    "app.preferences");
  accel_install_primary_action_accelerator (
    "<Control><Shift>question",
    "app.shortcuts");
  accel_install_primary_action_accelerator (
    "<Control>n",
    "win.new");
  accel_install_primary_action_accelerator (
    "<Control>o",
    "win.open");
  accel_install_primary_action_accelerator (
    "<Control>s",
    "win.save");
  accel_install_primary_action_accelerator (
    "<Control><Shift>s",
    "win.save-as");
  accel_install_primary_action_accelerator (
    "<Control>e",
    "win.export-as");
  accel_install_primary_action_accelerator (
    "<Control>z",
    "win.undo");
  accel_install_primary_action_accelerator (
    "<Control><Shift>z",
    "win.redo");
  accel_install_primary_action_accelerator (
    "<Control>x",
    "win.cut");
  accel_install_primary_action_accelerator (
    "<Control>c",
    "win.copy");
  accel_install_primary_action_accelerator (
    "<Control>v",
    "win.paste");
  accel_install_primary_action_accelerator (
    "Delete",
    "win.delete");
  accel_install_primary_action_accelerator (
    "<Control>backslash",
    "win.clear-selection");
  accel_install_primary_action_accelerator (
    "<Control>a",
    "win.select-all");
  accel_install_primary_action_accelerator (
    "<Control><Shift>4",
    "win.toggle-left-panel");
  accel_install_primary_action_accelerator (
    "<Control><Shift>6",
    "win.toggle-right-panel");
  accel_install_primary_action_accelerator (
    "<Control><Shift>2",
    "win.toggle-bot-panel");
  accel_install_primary_action_accelerator (
    "<Control>equal",
    "win.zoom-in");
  accel_install_primary_action_accelerator (
    "<Control>minus",
    "win.zoom-out");
  accel_install_primary_action_accelerator (
    "<Control>plus",
    "win.original-size");
  accel_install_primary_action_accelerator (
    "<Control>bracketleft",
    "win.best-fit");
  accel_install_primary_action_accelerator (
    "<Control>l",
    "win.loop-selection");
  accel_install_primary_action_accelerator (
    "1",
    "win.select-mode");
  accel_install_primary_action_accelerator (
    "2",
    "win.edit-mode");
  accel_install_primary_action_accelerator (
    "3",
    "win.cut-mode");
  accel_install_primary_action_accelerator (
    "4",
    "win.eraser-mode");
  accel_install_primary_action_accelerator (
    "5",
    "win.ramp-mode");
  accel_install_primary_action_accelerator (
    "6",
    "win.audition-mode");
  accel_install_action_accelerator (
    "KP_4", "BackSpace",
    "win.goto-prev-marker");
  accel_install_primary_action_accelerator (
    "KP_6",
    "win.goto-next-marker");
  accel_install_primary_action_accelerator (
    "space",
    "win.play-pause");
  accel_install_primary_action_accelerator (
    "Q",
    "win.quick-quantize::global");
  accel_install_primary_action_accelerator (
    "<Alt>Q",
    "win.quantize-options::global");
}

ZrythmApp *
zrythm_app_new (void)
{
  ZrythmApp * self =  g_object_new (
    ZRYTHM_APP_TYPE,
    "application-id", "org.zrythm.Zrythm",
    "resource-base-path", "/org/zrythm/Zrythm",
    "flags", G_APPLICATION_HANDLES_OPEN,
    NULL);

  self->zrythm = calloc (1, sizeof (Zrythm));
  ZRYTHM = self->zrythm;
  ZRYTHM->project = calloc (1, sizeof (Project));

  return self;
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
}

static void
zrythm_app_init (ZrythmApp * _app)
{
  g_message ("initing zrythm app");

#ifdef _WIN32
#else
  /* prefer x11 backend because plugin UIs need it to load */
  gdk_set_allowed_backends ("x11,*");
#endif

  const GActionEntry entries[] = {
    { "prompt_for_project", on_prompt_for_project },
    { "init_main_window", on_init_main_window },
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
    { "preferences", activate_preferences },
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
