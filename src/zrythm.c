/*
 * zrythm_app.c - The GTK app
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
#include "settings.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/start_assistant.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/io.h"

#include <gtk/gtk.h>


G_DEFINE_TYPE(ZrythmApp, zrythm_app, GTK_TYPE_APPLICATION);

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
static StartAssistantWidget * assistant;
static char * filename = NULL;
static TaskId * task_id; ///< current task;
static UpdateSplashData * data;

static void
update_splash (UpdateSplashData *data)
{
  splash_widget_update (splash,
                        data->message,
                        data->progress);
}

/**
 * Initializes/creates the default dirs/files.
 */
static void
init_dirs_and_files ()
{
  ZRYTHM->zrythm_dir = g_strdup_printf ("%s%sZrythm",
                                       io_get_home_dir (),
                                       io_get_separator ());
  io_mkdir (ZRYTHM->zrythm_dir);

  ZRYTHM->projects_dir = g_strdup_printf ("%s%sProjects",
                                         ZRYTHM->zrythm_dir,
                                         io_get_separator ());
  io_mkdir (ZRYTHM->projects_dir);

  ZRYTHM->recent_projects_file = g_strdup_printf ("%s%s.recent_projects",
                                                        ZRYTHM->zrythm_dir,
                                                        io_get_separator ());
}

/**
 * Initializes the array of recent projects in Zrythm app
 * FIXME use GtkRecentManager
 */
static void
init_recent_projects ()
{
  FILE * file = io_touch_file (ZRYTHM->recent_projects_file);

  char line[256];
  while (fgets(line, sizeof(line), file))
    {
      char * tmp = g_strdup_printf ("%s", line);
      char * project_filename = g_strstrip (tmp);
      FILE * project_file = fopen (project_filename, "r");
      if (project_file)
        {
          ZRYTHM->recent_projects [ZRYTHM->num_recent_projects++] =
            project_filename;
          fclose (project_file);
        }
  }

  fclose(file);
}

void
zrythm_add_to_recent_projects (ZrythmApp * self,
                                   const char * filepath)
{
  FILE * file = fopen (self->recent_projects_file, "a");

  fprintf (file, "%s\n", filepath);

  fclose(file);
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
        "Initializing settings";
      data->progress = 0.3;
      break;
    case TASK_INIT_SETTINGS:
      ZRYTHM->settings = settings_new ();
      data->message =
        "Initializing audio engine";
      data->progress = 0.4;
      break;
    case TASK_INIT_AUDIO_ENGINE:
      ZRYTHM->audio_engine = engine_new ();
      data->message =
        "Initializing plugin manager";
      data->progress = 0.6;
      break;
    case TASK_INIT_PLUGIN_MANAGER:
      ZRYTHM->plugin_manager = plugin_manager_new ();
      data->message =
        "Setting up backend";
      data->progress = 0.7;
      break;
    case TASK_END:
      engine_setup (AUDIO_ENGINE);
      plugin_manager_setup (PLUGIN_MANAGER);
      data->message =
        "Loading project";
      data->progress = 0.8;
      break;
    }
  g_idle_add ((GSourceFunc) update_splash, data);
}

static void
task_completed_cb (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
  if (*task_id == TASK_END)
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (zrythm),
        "prompt_for_project",
        NULL);
    }
  else
    {
      (*task_id)++;
      GTask * task = g_task_new (ZRYTHM,
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
static void on_setup_main_window (GAction  *action,
                             GVariant *parameter,
                             gpointer  user_data)
{
  g_message ("setup main window");

  main_window_widget_refresh (MAIN_WINDOW);

  free (data);
}

/**
 * Called after the main window has been initialized.
 *
 * Loads the project backend or creates the default one.
 * FIXME rename
 */
static void on_load_project (GAction  *action,
                             GVariant *parameter,
                             gpointer  user_data)
{
  g_message ("load_project");
  ZrythmApp * app = ZRYTHM_APP (user_data);

  app->project = project_new ();
  app->tracklist = tracklist_new ();

  project_setup (app->project,
                 app->open_filename);
  Track * track = MIXER->master->track;

  tracklist_setup (TRACKLIST);


  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm),
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
static void on_init_main_window (GAction  *action,
                             GVariant *parameter,
                             gpointer  data)
{
  g_message ("init main window");

  gtk_widget_destroy (GTK_WIDGET (splash));

  ZrythmApp * app = ZRYTHM_APP (data);
  app->entries[0].target = "PLUGIN_DESCR";
  app->entries[0].flags = GTK_TARGET_SAME_APP;
  app->entries[0].info = 0;
  app->num_entries = 1;
  app->main_window = main_window_widget_new (app);

  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm),
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
    G_ACTION_GROUP (zrythm),
    "init_main_window",
    NULL);
}


/**
 * Called before on_load_project.
 *
 * Checks if a project was given in the command line. If not,
 * it prompts the user for a project (start assistant).
 */
static void on_prompt_for_project (GAction  *action,
                             GVariant *parameter,
                             gpointer  data)
{
  g_message ("prompt for project");
  ZrythmApp * app = ZRYTHM_APP (data);

  if (app->open_filename)
    {
      g_action_group_activate_action (
        G_ACTION_GROUP (zrythm),
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
        start_assistant_widget_new (GTK_WINDOW(splash),
                                    1);
      g_signal_connect (G_OBJECT (assistant),
                        "apply",
                        G_CALLBACK (on_finish),
                        NULL);
      g_signal_connect (G_OBJECT (assistant),
                        "cancel",
                        G_CALLBACK (on_finish),
                        (void *) 1);
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
  g_message ("activate %d", *task_id);

  /*PROJECT->snap_grid_timeline.grid_auto = 1;*/
  /*[>PROJECT->snap_grid_timeline.grid_density = 3;<]*/
  /*PROJECT->snap_grid_timeline.grid_density = 1;*/
  /*PROJECT->snap_grid_timeline.snap_to_grid = 1;*/
  /*PROJECT->snap_grid_timeline.snap_to_edges = 1;*/
  /*PROJECT->snap_grid_timeline.type = SNAP_GRID_TIMELINE;*/
  /*PROJECT->snap_grid_midi.grid_auto = 1;*/
  /*[>PROJECT->snap_grid_midi.grid_density = 4;<]*/
  /*PROJECT->snap_grid_midi.grid_density = 1;*/
  /*PROJECT->snap_grid_midi.snap_to_grid = 1;*/
  /*PROJECT->snap_grid_midi.snap_to_edges = 1;*/
  /*PROJECT->snap_grid_midi.type = SNAP_GRID_MIDI;*/

  /*PROJECT = calloc (1, sizeof (Project));*/

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
  splash = splash_window_widget_new (ZRYTHM_APP (app));
  gtk_window_present (GTK_WINDOW (splash));

  /* start initialization task */
  task_id = calloc (1, sizeof (TaskId));
  *task_id = TASK_START;
  data = calloc (1, sizeof (UpdateSplashData));
  GTask * task = g_task_new (ZRYTHM,
                             NULL,
                             task_completed_cb,
                             (gpointer) task_id);
  g_task_set_task_data (task,
                        (gpointer) task_id,
                        NULL);
  g_task_run_in_thread (task, task_func);
}

ZrythmApp *
zrythm_new ()
{
  ZrythmApp * self =  g_object_new (
    ZRYTHM_APP_TYPE,
    "application-id", "org.zrythm",
    "flags", G_APPLICATION_HANDLES_OPEN,
    NULL);

  return self;
}

static void
zrythm_app_class_init (ZrythmAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = zrythm_app_activate;
  G_APPLICATION_CLASS (class)->startup = zrythm_app_startup;
  G_APPLICATION_CLASS (class)->open = zrythm_app_open;
}

static void
zrythm_app_init (ZrythmApp *app)
{
  /* prompt for project */
  GSimpleAction * prompt_for_project_action =
    g_simple_action_new ("prompt_for_project", NULL);
  g_signal_connect (prompt_for_project_action,
                    "activate",
                    G_CALLBACK (on_prompt_for_project),
                    app);
  g_action_map_add_action (
    G_ACTION_MAP (app),
    G_ACTION (prompt_for_project_action));

  /* init main window */
  GSimpleAction * init_main_window_action =
    g_simple_action_new ("init_main_window", NULL);
  g_signal_connect (init_main_window_action,
                    "activate",
                    G_CALLBACK (on_init_main_window),
                    app);
  g_action_map_add_action (
    G_ACTION_MAP (app),
    G_ACTION (init_main_window_action));

  /* setup main window */
  GSimpleAction * setup_main_window_action =
    g_simple_action_new ("setup_main_window", NULL);
  g_signal_connect (setup_main_window_action,
                    "activate",
                    G_CALLBACK (on_setup_main_window),
                    app);
  g_action_map_add_action (
    G_ACTION_MAP (app),
    G_ACTION (setup_main_window_action));

  /* load project */
  GSimpleAction * load_project_action =
    g_simple_action_new ("load_project", NULL);
  g_signal_connect (load_project_action,
                    "activate",
                    G_CALLBACK (on_load_project),
                    app);
  g_action_map_add_action (
    G_ACTION_MAP (app),
    G_ACTION (load_project_action));
}
