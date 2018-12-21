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
#include "utils/xml.h"

#include <gtk/gtk.h>

Project *
project_new ()
{
  Project * self = calloc (1, sizeof (Project));

  return self;
}

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
create_default (Project * self,
                Mixer *   mixer)
{
  g_assert (self);
  g_assert (mixer);

  if (self->loaded)
    tear_down (self);

  self->title = g_strdup (DEFAULT_PROJECT_NAME);

  /* add master channel to mixer */
  mixer_add_channel (
    mixer,
    channel_create (CT_MASTER, "Master"));

  /* create chord track */
  self->chord_track = chord_track_default ();
}

static void
load (Project *    self,
      const char * filename)
{
  if (self->loaded)
    tear_down (self);

  g_assert (filename);
  char * dir = io_get_dir (filename);
  update_paths (self, dir);

  xml_load_ports ();
  xml_load_regions ();
  xml_load_project ();
  mixer_load_plugins (MIXER);

  char * filepath_noext = io_file_strip_path (dir);
  self->title = filepath_noext;
  g_free (filepath_noext);

  g_free (dir);

  self->filename = filename;
}

/**
 * If project has a filename set, it loads that. Otherwise
 * it loads the default project.
 */
void
project_setup (Project * self,
               const char * filename)
{
  if (filename)
    load (self, filename);
  else
    create_default (self,
                    MIXER);
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

  xml_write_ports ();
  xml_write_regions ();
  xml_write_project ();

  zrythm_add_to_recent_projects (
    ZRYTHM,
    PROJECT->project_file_path);
  self->title = io_file_strip_path (dir);
}

void
project_add_region (Project * self,
                    Region *  region)
{
  array_append ((void **) self->regions,
                &self->num_regions,
                (void *) region);
}
