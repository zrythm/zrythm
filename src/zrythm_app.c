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
#include "plugins/plugin_manager.h"

#include <gtk/gtk.h>

struct _ZrythmApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(ZrythmApp, zrythm_app, GTK_TYPE_APPLICATION);

static void
zrythm_app_init (ZrythmApp *app)
{
}

static void
zrythm_app_activate (GApplication *app)
{
  MainWindowWidget * win;

  g_message ("Creating main window...");
  win = main_window_widget_new (ZRYTHM_APP (app));
  /*gtk_widget_show_all (GTK_WIDGET (win));*/
  /*gtk_window_present (GTK_WINDOW (win));*/
}

/**
 * initialization logic
 */
static void
zrythm_app_startup (GApplication* app)
{
  g_message ("Starting up...");

  /* init system */
  zrythm_system = malloc (sizeof (Zrythm_System));

  G_APPLICATION_CLASS (zrythm_app_parent_class)->startup (app);

  init_settings_manager ();

  init_widget_manager ();

  plugin_manager_init ();

  init_audio_engine ();

  // create project
  create_project ("project.xml");
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
  return g_object_new (ZRYTHM_APP_TYPE,
                       "application-id", "online.alextee.zrythm",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}
