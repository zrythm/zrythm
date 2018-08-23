/*
 * main.c - main
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

#include "init.h"

#include <gtk/gtk.h>

#include <suil/suil.h>

/**
 * main
 */
int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  /* init suil */
  suil_init(&argc, &argv, SUIL_ARG_NONE);

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
