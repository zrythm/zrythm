/*
 * init.c - initialization logic
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm_system.h"
#include "init.h"
#include "project.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/timeline.h"
#include "gui/main_window.h"
#include "plugins/plugin_manager.h"

/**
 * performs shutdown tasks
 */
static void
shutdown (GtkApplication* app,
          gpointer        user_data)
{

}

/**
 * opens files and shows them in a new window.
 * This corresponds to someone trying to open a document (or documents)
 * using the application from the file browser, or similar.
 */
static void
open (GtkApplication* app,
      gpointer        user_data)
{

}
/**
 * creates a window and shows it
 */
void
activate (GtkApplication* app,
          gpointer        user_data)
{
  // create main window and show it
  g_message ("Creating main window...");
  GtkWidget * window = create_main_window();
  gtk_widget_show_all (window);

  // start main loop
  gtk_main ();
}

/**
 * initialization logic
 */
void
startup (GtkApplication* app,
         gpointer        user_data)
{
  /* init system */
  zrythm_system_init ();

  // create project
  create_project ("project.xml");

}
