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
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2_plugin.h"
#include "utils/io.h"
#include "utils/smf.h"
#include "utils/xml.h"

#include <gtk/gtk.h>


void
project_create (char * filename)
{

  // set title
  g_message ("Creating project %s...", filename);
  PROJECT->title = g_strdup (filename);

  PROJECT->snap_grid_timeline.grid_auto = 1;
  /*PROJECT->snap_grid_timeline.grid_density = 3;*/
  PROJECT->snap_grid_timeline.grid_density = 1;
  PROJECT->snap_grid_timeline.snap_to_grid = 1;
  PROJECT->snap_grid_timeline.snap_to_edges = 1;
  PROJECT->snap_grid_timeline.type = SNAP_GRID_TIMELINE;
  PROJECT->snap_grid_midi.grid_auto = 1;
  /*PROJECT->snap_grid_midi.grid_density = 4;*/
  PROJECT->snap_grid_midi.grid_density = 1;
  PROJECT->snap_grid_midi.snap_to_grid = 1;
  PROJECT->snap_grid_midi.snap_to_edges = 1;
  PROJECT->snap_grid_midi.type = SNAP_GRID_MIDI;

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
  g_strfreev (parts);
  char * separator = io_get_separator ();
  char ** path_file = g_strsplit (file_part, separator, -1);
  int i = 0;
  while (path_file[i] != NULL)
    i++;
  char * path = "";
  for (int j = 0; j < i - 1; j++)
    {
      char * tmp = path;
      path = g_strconcat (tmp, path_file[j]);
      g_free (tmp);
    }
  g_strfreev (path_file);

  char * tmp = g_strdup_printf ("%s_ports.xml", filename);
  xml_write_ports (tmp);
  g_free (tmp);

  /*  write regions & their corresponding MIDI */
  smf_save_regions (filename);
  tmp = g_strdup_printf ("%s_regions.xml", filename);
  xml_write_regions (tmp);
  g_free (tmp);

  /* write plugin states */
  char * dir = io_get_dir (filename);
  io_mkdir (dir);
  char * state_dir = g_strdup_printf ("%s%sstates",
                                      dir,
                                      separator);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      char * state_dir_channel = g_strdup_printf ("%s%s%d",
                                          state_dir,
                                          separator,
                                          channel->id);
      for (int j = 0; j < MAX_PLUGINS; j++)
        {
          Plugin * plugin = channel->strip[j];

          if (plugin)
            {
              if (plugin->descr->protocol == PROT_LV2)
                {
                  char * state_dir_plugin = g_strdup_printf ("%s%s%d",
                                              state_dir_channel,
                                              separator,
                                              j);
                  LV2_Plugin * lv2_plugin = (LV2_Plugin *) plugin->original_plugin;
                  g_message ("symap size before saving state %d",
                             lv2_plugin->symap->size);
                  lv2_save_state (lv2_plugin,
                                  state_dir_plugin);
                  g_free (state_dir_plugin);
                }
              else
                {
                  //
                }
            }
        }
      g_free (state_dir_channel);
    }

  tmp = g_strdup_printf ("%s.xml", filename);
  xml_write_project (tmp);
  g_free (tmp);

  PROJECT->path = path;
  project_set_title (filename);

  g_free (dir);
  g_free (separator);
  g_free (state_dir);
}

/**
 * Sets title to project and main window
 */
void
project_set_title (char * title)
{
  PROJECT->title = title;

  if (MAIN_WINDOW)
    {
      char * title = g_strdup_printf ("Zrythm - %s", PROJECT->title);
      gtk_label_set_text (MAIN_WINDOW->title, title);
      g_free (title);
    }
}

void
project_load (char * filename)
{

}
