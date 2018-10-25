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
#include "audio/track.h"
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
}

void
project_create_default ()
{
  project_create (DEFAULT_PROJECT_NAME);
}

void
project_update_paths (const char * dir)
{
  PROJECT->dir = g_strdup (dir);
  PROJECT->project_file_path =
    g_strdup_printf ("%s%s%s",
                     PROJECT->dir,
                     io_get_separator (),
                     PROJECT_FILE);
  PROJECT->regions_file_path =
    g_strdup_printf ("%s%s%s",
                     PROJECT->dir,
                     io_get_separator (),
                     PROJECT_REGIONS_FILE);
  PROJECT->ports_file_path =
    g_strdup_printf ("%s%s%s",
                     PROJECT->dir,
                     io_get_separator (),
                     PROJECT_PORTS_FILE);
  PROJECT->regions_dir =
    g_strdup_printf ("%s%s%s",
                     PROJECT->dir,
                     io_get_separator (),
                     PROJECT_REGIONS_DIR);
  PROJECT->states_dir =
    g_strdup_printf ("%s%s%s",
                     PROJECT->dir,
                     io_get_separator (),
                     PROJECT_STATES_DIR);
  g_message ("updated paths %s", PROJECT->dir);
}

void
project_save (char * dir)
{
  io_mkdir (dir);
  project_update_paths (dir);

  smf_save_regions ();

  /* write plugin states */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = channel->strip[j];

          if (plugin)
            {
              if (plugin->descr->protocol == PROT_LV2)
                {
                  char * state_dir_plugin = g_strdup_printf ("%s%s%s_%d",
                                                             PROJECT->states_dir,
                                                             io_get_separator (),
                                                             channel->name,
                                                             j);
                  LV2_Plugin * lv2_plugin = (LV2_Plugin *) plugin->original_plugin;
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
    }

  xml_write_ports ();
  xml_write_regions ();
  xml_write_project ();

  zrythm_app_add_to_recent_projects (PROJECT->project_file_path);
  project_set_title (io_file_strip_path (dir));
}

/**
 * Sets title to project and main window
 */
void
project_set_title (char * _title)
{
  PROJECT->title = g_strdup (_title);

  if (MAIN_WINDOW)
    {
      char * title = g_strdup_printf ("Zrythm - %s", PROJECT->title);
      gtk_label_set_text (MAIN_WINDOW->title, title);
      g_free (title);
    }
}

void
project_load (char * filepath) ///< this is the xml file
{
  if (filepath)
    {
      char * dir = io_get_dir (filepath);
      project_update_paths (dir);

      xml_load_ports ();
      xml_load_regions ();
      xml_load_project ();
      mixer_load_plugins ();

      char * filepath_noext = io_file_strip_path (dir);
      project_set_title (filepath_noext);
      g_free (filepath_noext);

      g_free (dir);
    }
  else
    {
      MIXER->master = channel_create_master ();
      ADD_CHANNEL (channel_create (CT_MIDI, "Ch 1"));
    }

  /* generate automatables */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      track_generate_automatables (MIXER->channels[i]->track);
    }
  track_generate_automatables (MIXER->master->track);

}

