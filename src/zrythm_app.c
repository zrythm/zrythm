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

static void
zrythm_app_update_splash (const char * message,
                          int        progress)
{
  splash_widget_update (splash,
                        message,
                        progress);
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

static void
init_recent_projects ()
{
  FILE * file = io_touch_file (G_ZRYTHM_APP->recent_projects_file);

  char line[256];
  while (fgets(line, sizeof(line), file))
    {
      char * tmp = g_strdup_printf ("%s", line);
      char * project_filename = g_strstrip (tmp);
      /*g_free (tmp);*/
      G_ZRYTHM_APP->recent_projects [G_ZRYTHM_APP->num_recent_projects++] =
        project_filename;
  }
  g_message (G_ZRYTHM_APP->recent_projects_file);

  fclose(file);
}

static void
init_after_setting_project ()
{
  init_settings_manager ();

  init_widget_manager ();

  plugin_manager_init ();

  init_audio_engine ();

  g_message ("Creating main window...");
  gtk_window_close (GTK_WINDOW (splash));
  main_window_widget_new (ZRYTHM_APP (app));
}

static void
on_apply (GtkAssistant * assistant,
          gpointer       user_data)
{
  /* TODO create/load project here */

  init_after_setting_project ();
}

static void
zrythm_app_activate (GApplication * _app)
{
  app = _app;
  splash = splash_window_widget_new (ZRYTHM_APP (app));
  gtk_window_present (GTK_WINDOW (splash));

  zrythm_app_update_splash ("Starting up", 0);

  AUDIO_ENGINE = calloc (1, sizeof (Audio_Engine));
  PROJECT = calloc (1, sizeof (Project));

  /* init zrythm folders ~/Zrythm */
  zrythm_app_update_splash ("Initializing files and folders", 0);
  init_dirs_and_files ();
  init_recent_projects ();

  /* show the assistant */
  zrythm_app_update_splash ("Waiting for project...", 0);
  StartAssistantWidget * assistant =
    start_assistant_widget_new (GTK_WINDOW(splash),
                                1);
  g_signal_connect (G_OBJECT (assistant),
                    "apply",
                    G_CALLBACK (on_apply),
                    app);
  gtk_window_present (assistant);
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

