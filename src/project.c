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

#include "zrythm.h"
#include "project.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/smf.h"

#include <gtk/gtk.h>

/**
 * Tears down the project before loading another one.
 */
static void
tear_down (Project * self)
{

}

static void
update_paths (Project * self,
              const char * dir)
{
  PROJECT->dir = g_strdup (dir);
  PROJECT->project_file_path =
    g_build_filename (PROJECT->dir,
                      PROJECT_FILE,
                      NULL);
  PROJECT->regions_file_path =
    g_build_filename (PROJECT->dir,
                      PROJECT_REGIONS_FILE,
                      NULL);
  PROJECT->ports_file_path =
    g_build_filename (PROJECT->dir,
                      PROJECT_PORTS_FILE,
                      NULL);
  PROJECT->regions_dir =
    g_build_filename (PROJECT->dir,
                      PROJECT_REGIONS_DIR,
                      NULL);
  PROJECT->states_dir =
    g_build_filename (PROJECT->dir,
                      PROJECT_STATES_DIR,
                      NULL);
  g_message ("updated paths %s", PROJECT->dir);
}

void
create_default (Project * self)
{
  engine_init (&self->audio_engine);
  undo_manager_init (&self->undo_manager);

  self->title = g_strdup (DEFAULT_PROJECT_NAME);

  /* add master channel to mixer */
  mixer_add_channel (
    channel_create (CT_MASTER, "Master"));

  /* create chord track */
  self->chord_track = chord_track_default ();

  self->loaded = 1;
}

static void
load (Project *    self,
      char * filename)
{
  if (self->loaded)
    tear_down (self);

  g_assert (filename);
  char * dir = io_get_dir (filename);
  update_paths (self, dir);

  /*xml_load_ports ();*/
  /*xml_load_regions ();*/
  /*xml_load_project ();*/
  mixer_load_plugins ();

  char * filepath_noext = g_path_get_basename (dir);
  self->title = filepath_noext;
  g_free (filepath_noext);

  g_free (dir);

  self->filename = filename;

  self->loaded = 1;
}

/**
 * If project has a filename set, it loads that. Otherwise
 * it loads the default project.
 */
void
project_load (char * filename)
{
  if (filename)
    load (PROJECT, filename);
  else
    create_default (PROJECT);

  snap_grid_init (&PROJECT->snap_grid_timeline,
                  NOTE_LENGTH_1_1);
  quantize_init (&PROJECT->quantize_timeline,
                 NOTE_LENGTH_1_1);
  snap_grid_init (&PROJECT->snap_grid_midi,
                  NOTE_LENGTH_1_8);
  quantize_init (&PROJECT->quantize_midi,
                NOTE_LENGTH_1_8);
  piano_roll_init (&PROJECT->piano_roll);
  snap_grid_update_snap_points (&PROJECT->snap_grid_timeline);
  snap_grid_update_snap_points (&PROJECT->snap_grid_midi);
  quantize_update_snap_points (&PROJECT->quantize_timeline);
  quantize_update_snap_points (&PROJECT->quantize_midi);
  tracklist_init (&PROJECT->tracklist);
}


void
project_save (Project *  self,
              const char * dir)
{
  io_mkdir (dir);
  update_paths (self, dir);

  smf_save_regions ();

  /* write plugin states */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = channel->plugins[j];

          if (plugin)
            {
              if (plugin->descr->protocol == PROT_LV2)
                {
                  char * tmp =
                    g_strdup_printf ("%s_%d",
                                     channel->name,
                                     j);
                  char * state_dir_plugin =
                    g_build_filename (PROJECT->states_dir,
                                      tmp,
                                      NULL);
                  g_free (tmp);

                  Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->original_plugin;
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

  /*xml_write_ports ();*/
  /*xml_write_regions ();*/
  /*xml_write_project ();*/

  zrythm_add_to_recent_projects (
    ZRYTHM,
    PROJECT->project_file_path);
  self->title = g_path_get_basename (dir);
}

void
project_add_region (Project * self,
                    Region *  region)
{
  array_append ((void **) self->regions,
                &self->num_regions,
                (void *) region);
}
