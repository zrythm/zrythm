/*
 * project.c - A project (or song), containing all the project data
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

#include "zrythm_app.h"
#include "project.h"
#include "audio/engine.h"
#include "audio/transport.h"
#include "utils/xml.h"

#include <gtk/gtk.h>


void
project_setup (char * filename)
{

  // set title
  g_message ("Setting up project %s...", filename);
  PROJECT->title = g_strdup (filename);

  transport_init ();
  AUDIO_ENGINE->run = 1;
}

void
project_save (char * filename)
{
  /* write XML */
  char ** parts = g_strsplit (filename, ".", 2);
  char * file_part = parts[0];
  char * ext_part = parts[1];
  char * tmp = g_strdup_printf ("%s_ports.%s", file_part, ext_part);
  xml_write_ports (tmp);
  g_free (tmp);

  /* TODO write regions & their corresponding MIDI */

  /* TODO write plugin states */

  tmp = g_strdup_printf ("%s.%s", file_part, ext_part);
  xml_write_project (tmp);
  g_free (tmp);

}

void
project_load (char * filename)
{

}
