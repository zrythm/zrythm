/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm.h"
#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/quantize.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/project_assistant.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/localization.h"
#include "utils/io.h"

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
static ProjectAssistantWidget * assistant;
static TaskId * task_id; ///< current task;
static UpdateSplashData * data;

static int
update_splash (UpdateSplashData *data)
{
  /* sometimes this gets called after the splash window
   * gets deleted so added for safety */
  if (splash && GTK_IS_WIDGET (splash))
    {
      splash_widget_update (splash,
                            data->message,
                            data->progress);
    }
  return G_SOURCE_REMOVE;
}

/**
 * Initializes/creates the default dirs/files.
 */
static void
init_dirs_and_files ()
{
  ZRYTHM->zrythm_dir =
    g_build_filename (io_get_home_dir (),
                      "zrythm",
                      NULL);
  io_mkdir (ZRYTHM->zrythm_dir);

  ZRYTHM->projects_dir =
    g_build_filename (ZRYTHM->zrythm_dir,
                      "Projects",
                      NULL);
  io_mkdir (ZRYTHM->projects_dir);
}

/**
 * Initializes the array of recent projects in Zrythm app
 * FIXME use GtkRecentManager
 */
static void
init_recent_projects ()
{
  gchar ** recent_projects =
    g_settings_get_strv (S_GENERAL,
                         "recent-projects");

  /* get recent projects */
  ZRYTHM->num_recent_projects = 0;
  while (recent_projects[
           ZRYTHM->num_recent_projects] != NULL)
    {
      ZRYTHM->recent_projects[
        ZRYTHM->num_recent_projects++] =
          recent_projects[
            ZRYTHM->num_recent_projects];
    }
}

void
zrythm_add_to_recent_projects (Zrythm * self,
                               const char * filepath)
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
            (const char * const *) ZRYTHM->recent_projects);
        }

    }

}

static void
task_func (GTask *task,
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
      settings_init (&ZRYTHM->settings);
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
      file_manager_init (&ZRYTHM->file_manager);
      data->message =
        _("Setting up backend");
      data->progress = 0.7;
      break;
    case TASK_END:
      plugin_manager_scan_plugins (&ZRYTHM->plugin_manager);
      file_manager_load_files (&ZRYTHM->file_manager);
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
static void on_setup_main_window (GSimpleAction  *action,
                             GVariant *parameter,
                             gpointer  user_data)
{
  g_message ("setup main window");

  events_init (&ZRYTHM->events);
  main_window_widget_refresh (MAIN_WINDOW);

  AUDIO_ENGINE->run = 1;

  free (data);
}

/**
 * Called after the main window has been initialized.
 *
 * Loads the project backend or creates the default one.
 * FIXME rename
 */
static void on_load_project (GSimpleAction  *action,
                             GVariant *parameter,
                             gpointer  user_data)
{
  g_message ("load_project");

  project_load (ZRYTHM->open_filename);

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
static void on_init_main_window (GSimpleAction  *action,
                             GVariant *parameter,
                             gpointer  data)
{
  g_message ("init main window");

  gtk_widget_destroy (GTK_WIDGET (splash));

  ZrythmApp * app = ZRYTHM_APP (data);
  ZRYTHM->main_window = main_window_widget_new (app);

  g_action_group_activate_action (
    G_ACTION_GROUP (app),
    "load_project",
    NULL);
}

static void
on_finish (GtkAssistant * _assistant,
          gpointer       user_data)
{
  if (user_data) /* if cancel */
    {
      gtk_widget_destroy (GTK_WIDGET (assistant));
      zrythm->open_filename = NULL;
    }
  else if (!assistant->selection) /* if apply to create new project */
    {
      zrythm->open_filename = NULL;
    }
  else /* if apply to load project */
    {
      zrythm->open_filename = assistant->selection->filename;
    }
  gtk_widget_set_visible (GTK_WIDGET (assistant), 0);
  data->message =
    "Finishing";
  data->progress = 1.0;
  update_splash (data);
  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app),
    "init_main_window",
    NULL);
}


/**
 * Called before on_load_project.
 *
 * Checks if a project was given in the command line. If not,
 * it prompts the user for a project (start assistant).
 */
static void on_prompt_for_project (GSimpleAction  *action,
                             GVariant *parameter,
                             gpointer  data)
{
  g_message ("prompt for project");
  ZrythmApp * app = ZRYTHM_APP (data);

  if (ZRYTHM->open_filename)
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (app),
        "init_main_window",
        NULL);
    }
  else
    {
      /* init zrythm folders ~/Zrythm */
      init_dirs_and_files ();
      init_recent_projects ();

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
      gtk_window_present (GTK_WINDOW (assistant));
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
zrythm_app_open (GApplication  *app,
             GFile        **files,
             gint           n_files,
             const gchar   *hint)
{
  g_assert (n_files == 1);

  GFile * file = files[0];
  zrythm->open_filename = g_file_get_path (file);
  g_message ("open %s", zrythm->open_filename);
}

/**
 * First function that gets called.
 */
static void
zrythm_app_startup (GApplication* _app)
{
  G_APPLICATION_CLASS (zrythm_app_parent_class)->startup (_app);

  app = _app;

  /* show splash screen */
  splash =
    splash_window_widget_new (ZRYTHM_APP (app));
  gtk_window_present (
    GTK_WINDOW (splash));

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
    "<Control>q",
    "app.quit");
  accel_install_primary_action_accelerator (
    "F11",
    "app.fullscreen");
  accel_install_primary_action_accelerator (
    "<Control><Shift>p",
    "app.preferences");
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
    "<Control><Shift>a",
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
    "win.eraser-mode");
  accel_install_primary_action_accelerator (
    "4",
    "win.ramp-mode");
  accel_install_primary_action_accelerator (
    "5",
    "win.audition-mode");
}

ZrythmApp *
zrythm_app_new ()
{
  ZrythmApp * self =  g_object_new (
    ZRYTHM_APP_TYPE,
    "application-id", "org.zrythm",
    "resource-base-path", "/org/zrythm",
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
zrythm_app_init (ZrythmApp *app)
{
  /* prefer x11 backend because plugin UIs need it to load */
  gdk_set_allowed_backends ("x11,*");

  const GActionEntry entries[] = {
    { "prompt_for_project", on_prompt_for_project },
    { "init_main_window", on_init_main_window },
    { "setup_main_window", on_setup_main_window },
    { "load_project", on_load_project },
    { "about", activate_about },
    { "fullscreen", activate_fullscreen },
    { "manual", activate_manual },
    { "forums", activate_forums },
    { "bugreport", activate_bugreport },
    { "donate", activate_donate },
    { "iconify", activate_iconify },
    { "preferences", activate_preferences },
    { "quit", activate_quit },
    { "shortcuts", activate_shortcuts },
  };

  g_action_map_add_action_entries (
    G_ACTION_MAP (app),
    entries,
    G_N_ELEMENTS (entries),
    app);
}
