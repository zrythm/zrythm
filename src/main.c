/*
 * main.cpp - main
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

#include <gtk/gtk.h>
#include "init.h"


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
 * main
 */
int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  // create GTK application
  app = gtk_application_new ("online.alextee.zrythm", G_APPLICATION_FLAGS_NONE);

  // setup signals
  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  // sends activate signal
  status = g_application_run (G_APPLICATION (app), argc, argv);

  // free memory
  g_object_unref (app);

  return status;
}
