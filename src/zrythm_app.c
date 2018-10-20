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

#include "zrythm_app.h"
#include "settings_manager.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "gui/widget_manager.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/start_assistant.h"
#include "plugins/plugin_manager.h"
#include "utils/io.h"

#include <gtk/gtk.h>


G_DEFINE_TYPE(ZrythmApp, zrythm_app, GTK_TYPE_APPLICATION);

static SplashWindowWidget * splash;
static GApplication * app;
static StartAssistantWidget * assistant;

typedef struct UpdateSplashData
{
  const char * message;
  gdouble progress;
} UpdateSplashData;

static void
zrythm_app_update_splash (UpdateSplashData *data)
{

  splash_widget_update (splash,
                        data->message,
                        data->progress);
}

static void
zrythm_app_init (ZrythmApp *app)
{
}

static char *
load_or_create_project (GtkWindow * parent)
{
  GtkWidget *dialog, *label, *content_area;
 GtkDialogFlags flags;

 // Create the widgets
 flags = GTK_DIALOG_DESTROY_WITH_PARENT;
 dialog = gtk_dialog_new_with_buttons ("Load existing project",
                                       parent,
                                       flags,
                                       "_Load",
                                       GTK_RESPONSE_YES,
                                       "_New",
                                       GTK_RESPONSE_NO,
                                       NULL);
 content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
 label = gtk_label_new ("message");

 // Ensure that the dialog box is destroyed when the user responds

 g_signal_connect_swapped (dialog,
                           "response",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog);

 // Add the label, and show everything we’ve added

 gtk_container_add (GTK_CONTAINER (content_area), label);
 gtk_widget_show_all (dialog);

  int result = gtk_dialog_run (GTK_DIALOG (dialog));
  switch (result)
    {
      case GTK_RESPONSE_YES:
          {
        GtkDialog * open_dialog = dialogs_get_open_project_dialog (parent);

        int res = gtk_dialog_run (open_dialog);
        if (res == GTK_RESPONSE_ACCEPT)
          {
            char *filename;
            GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
            filename = gtk_file_chooser_get_filename (chooser);
            project_load (filename);
            g_free (filename);
          }
        else
          {
            project_create_default ();
          }

        gtk_widget_destroy (open_dialog);
          }
        break;
      case GTK_RESPONSE_NO:

        break;
      default:
        project_create_default();
        break;
    }
  gtk_widget_destroy (dialog);

}

/**
 * Initializes/creates the default dirs/files.
 */
static void
init_dirs_and_files ()
{
  G_ZRYTHM_APP->zrythm_dir = g_strdup_printf ("%s%sZrythm",
                                       io_get_home_dir (),
                                       io_get_separator ());
  io_mkdir (G_ZRYTHM_APP->zrythm_dir);

  G_ZRYTHM_APP->projects_dir = g_strdup_printf ("%s%sProjects",
                                         G_ZRYTHM_APP->zrythm_dir,
                                         io_get_separator ());
  io_mkdir (G_ZRYTHM_APP->projects_dir);

  G_ZRYTHM_APP->recent_projects_file = g_strdup_printf ("%s%s.recent_projects",
                                                        G_ZRYTHM_APP->zrythm_dir,
                                                        io_get_separator ());
}

/**
 * Initializes the array of recent projects in Zrythm app
 */
static void
init_recent_projects ()
{
  FILE * file = io_touch_file (G_ZRYTHM_APP->recent_projects_file);

  char line[256];
  while (fgets(line, sizeof(line), file))
    {
      char * tmp = g_strdup_printf ("%s", line);
      char * project_filename = g_strstrip (tmp);
      G_ZRYTHM_APP->recent_projects [G_ZRYTHM_APP->num_recent_projects++] =
        project_filename;
  }

  fclose(file);
}

static int finished[6] = { 0, 0, 0, 0 ,0 ,0 };

static void
task_func (GTask *task,
                    gpointer source_object,
                    gpointer task_data,
                    GCancellable *cancellable)
{
  int td = task_data;
  g_message ("running task func %d", td);

  switch (td)
    {
    case 0:
      project_create_default ();
      finished[0] = 1;
      break;
    case 1:
      while (finished[0] == 0)
        {
        }
      init_settings_manager ();
      finished[1] = 1;
      break;
    case 2:
      while (finished[1] == 0)
        {
        }
      init_widget_manager ();
      finished[2] = 1;
      break;
    case 3:
      while (finished[2] == 0)
        {
        }
      plugin_manager_init ();
      finished[3] = 1;
      break;
    case 4:
      while (finished[3] == 0)
        {
        }
      init_audio_engine ();
      finished[4] = 1;
      break;
    case 5:
      while (finished[4] == 0)
        {
        }
      finished[5] = 1;
      break;
    }
}

void
task_completed_cb (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
  int td = user_data;
  g_message ("task completed %d", td);

  UpdateSplashData * data = calloc (1, sizeof (UpdateSplashData));
  switch (td)
    {
    case 0:
      data->message = "Initializing settings manager";
      data->progress = 0.3;
      zrythm_app_update_splash (data);
      break;
    case 1:
      data->message = "Initializing widget manager";
      data->progress = 0.4;
      zrythm_app_update_splash (data);
      break;
    case 2:
      data->message = "Initializing plugin manager";
      data->progress = 0.5;
      zrythm_app_update_splash (data);
      break;
    case 3:
      data->message = "Initializing audio engine";
      data->progress = 0.7;
      zrythm_app_update_splash (data);
      break;
    case 4:
      data->message = "Creating main window";
      data->progress = 0.9;
      zrythm_app_update_splash (data);
      break;
    case 5:
      gtk_widget_destroy (GTK_WINDOW (splash));
      main_window_widget_new (ZRYTHM_APP (app));
      break;
    }

  free (data);
}

static void
on_apply (GtkAssistant * assistant,
          gpointer       user_data)
{
  StartAssistantWidget * sa = START_ASSISTANT_WIDGET (assistant);

  /* TODO create/load project here */
  if (sa->selection)
    {
      /*zrythm_app_update_splash ("Initializing settings manager", 0.3);*/
      project_load (sa->selection->filename);
    }

  /*init_after_setting_project ();*/
}

static void
on_cancel (GtkAssistant * assistant,
          gpointer       user_data)
{
  gtk_widget_destroy (GTK_WIDGET (assistant));
  GTask * task = g_task_new (G_ZRYTHM_APP,
                             NULL,
                             task_completed_cb,
                             0);
  g_task_set_task_data (task,
                        0,
                        NULL);
  g_task_run_in_thread (task, task_func);
  task = g_task_new (G_ZRYTHM_APP,
                     NULL,
                     task_completed_cb,
                     1);
  g_task_set_task_data (task,
                        1,
                        NULL);
  g_task_run_in_thread (task, task_func);
  task = g_task_new (G_ZRYTHM_APP,
                     NULL,
                     task_completed_cb,
                     2);
  g_task_set_task_data (task,
                        2,
                        NULL);
  g_task_run_in_thread (task, task_func);
  task = g_task_new (G_ZRYTHM_APP,
                     NULL,
                     task_completed_cb,
                     3);
  g_task_set_task_data (task,
                        3,
                        NULL);
  g_task_run_in_thread (task, task_func);
  task = g_task_new (G_ZRYTHM_APP,
                     NULL,
                     task_completed_cb,
                     4);
  g_task_set_task_data (task,
                        4,
                        NULL);
  g_task_run_in_thread (task, task_func);
  task = g_task_new (G_ZRYTHM_APP,
                     NULL,
                     task_completed_cb,
                     5);
  g_task_set_task_data (task,
                        5,
                        NULL);
  g_task_run_in_thread (task, task_func);

  g_message ("tasks scheduled");

  UpdateSplashData * data = calloc (1, sizeof (UpdateSplashData));
  data->message = "Creating project";
  data->progress = 0.3;
  zrythm_app_update_splash (data);
}

static void
zrythm_app_activate (GApplication * _app)
{
  app = _app;
  splash = splash_window_widget_new (ZRYTHM_APP (app));
  gtk_window_present (GTK_WINDOW (splash));

  UpdateSplashData * data = calloc (1, sizeof (UpdateSplashData));

  AUDIO_ENGINE = calloc (1, sizeof (Audio_Engine));
  PROJECT = calloc (1, sizeof (Project));

  /* init zrythm folders ~/Zrythm */
  init_dirs_and_files ();
  init_recent_projects ();

  /* show the assistant */
  assistant =
    start_assistant_widget_new (GTK_WINDOW(splash),
                                1);
  g_signal_connect (G_OBJECT (assistant),
                    "apply",
                    G_CALLBACK (on_apply),
                    assistant);
  g_signal_connect (G_OBJECT (assistant),
                    "cancel",
                    G_CALLBACK (on_cancel),
                    assistant);
  gtk_window_present (assistant);

  data->message = "Initializing...";
  data->progress = 0.01;
  zrythm_app_update_splash (data);
}

/**
 * initialization logic
 */
static void
zrythm_app_startup (GApplication* app)
{
  G_APPLICATION_CLASS (zrythm_app_parent_class)->startup (app);
}

static void
zrythm_app_open (GApplication  *app,
                  GFile        **files,
                  gint           n_files,
                  const gchar   *hint)
{
  /*GList *windows;*/
  /*MainWindowWidget *win;*/
  /*int i;*/

  /*windows = gtk_application_get_windows (GTK_APPLICATION (app));*/
  /*if (windows)*/
    /*win = MAIN_WINDOW_WIDGET (windows->data);*/
  /*else*/
    /*win = main_window_widget_new (ZRYTHM_APP (app));*/

  /*for (i = 0; i < n_files; i++)*/
    /*main_window_widget_open (win, files[i]);*/

  /*gtk_window_present (GTK_WINDOW (win));*/
}

static void
zrythm_app_class_init (ZrythmAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = zrythm_app_activate;
  G_APPLICATION_CLASS (class)->startup = zrythm_app_startup;
  G_APPLICATION_CLASS (class)->open = zrythm_app_open;
}

ZrythmApp *
zrythm_app_new (void)
{
  G_ZRYTHM_APP =  g_object_new (ZRYTHM_APP_TYPE,
                       "application-id", "online.alextee.zrythm",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
  return G_ZRYTHM_APP;
}

