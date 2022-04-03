/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#include "utils/log.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  LOG = log_new ();

  /* send activate signal */
  zrythm_app =
    zrythm_app_new (argc, (const char **) argv);

  int ret = g_application_run (
    G_APPLICATION (zrythm_app), argc, argv);
  g_object_unref (zrythm_app);

  log_free (LOG);

  return ret;
}
