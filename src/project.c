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
#include "audio/automation_curve.h"
#include "audio/automation_point.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/midi_note.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/smf.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * Tears down the project before loading another one.
 */
static void
tear_down (Project * self)
{

}

static void
update_paths (const char * dir)
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
  PROJECT->exports_dir =
    g_build_filename (PROJECT->dir,
                      PROJECT_EXPORTS_DIR,
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
  update_paths (dir);

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

/**
 * Sets if the project has range and updates UI.
 */
void
project_set_has_range (int has_range)
{
  PROJECT->has_range = has_range;

  timeline_ruler_refresh ();
}

void
project_save (const char * dir)
{
  io_mkdir (dir);
  update_paths (dir);
  PROJECT->title = io_path_get_basename (dir);

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
  char * yaml = project_serialize (PROJECT);
  g_message ("\n%s", yaml);

  zrythm_add_to_recent_projects (
    ZRYTHM,
    PROJECT->project_file_path);
  PROJECT->title = g_path_get_basename (dir);

  ui_show_notification ("Project saved.");
}

static int
get_next_available_id (void ** array,
                       int     size)
{
  for (int i = 0; i < size; i++)
    {
      /* if region doesn't exist at this index, use it */
      if (!array[i])
        return i;
    }
  return size;
}

#define PROJECT_ADD_X(camelcase, lowercase, plural) \
  void \
  project_add_##lowercase (camelcase * x) \
  { \
    x->id = \
      get_next_available_id ((void **) PROJECT->plural, \
                             PROJECT->num_##plural); \
    PROJECT->plural[x->id] = x; \
    PROJECT->num_##plural++; \
  }
#define PROJECT_GET_X(camelcase, lowercase, plural) \
  camelcase * \
  project_get_##lowercase (int id) \
  { \
    return PROJECT->plural[id]; \
  }


PROJECT_ADD_X (Region, region, regions)
PROJECT_GET_X (Region, region, regions)
PROJECT_ADD_X (Track, track, tracks)
PROJECT_GET_X (Track, track, tracks)
PROJECT_ADD_X (Channel, channel, channels)
PROJECT_GET_X (Channel, channel, channels)
PROJECT_ADD_X (Plugin, plugin, plugins)
PROJECT_GET_X (Plugin, plugin, plugins)
PROJECT_ADD_X (AutomationPoint, automation_point, automation_points)
PROJECT_GET_X (AutomationPoint, automation_point, automation_points)
PROJECT_ADD_X (AutomationCurve, automation_curve, automation_curves)
PROJECT_GET_X (AutomationCurve, automation_curve, automation_curves)
PROJECT_ADD_X (MidiNote, midi_note, midi_notes)
PROJECT_GET_X (MidiNote, midi_note, midi_notes)
PROJECT_ADD_X (Port, port, ports)
PROJECT_GET_X (Port, port, ports)

#undef PROJECT_ADD_X
#undef PROJECT_GET_X

SERIALIZE_SRC (Project, project)
DESERIALIZE_SRC (Project, project)
PRINT_YAML_SRC (Project, project)
