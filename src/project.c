/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * A project contains everything that should be
 * serialized.
 */

#include "zrythm.h"
#include "project.h"
#include "audio/automation_curve.h"
#include "audio/automation_lane.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
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
#include "gui/widgets/splash.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "utils/arrays.h"
#include "utils/general.h"
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
  g_message ("tearing down the project...");

  PROJECT->loaded = 0;

  if (self->title)
    g_free (self->title);

  engine_tear_down ();

  for (int i = 0; i < PROJECT->num_channels; i++)
    channel_free (PROJECT->channels[i]);

  track_free (PROJECT->chord_track);

  free (self);
}

static void
update_paths (const char * dir)
{
  if (PROJECT->filename)
    g_free (PROJECT->filename);
  if (PROJECT->dir)
    g_free (PROJECT->dir);
  if (PROJECT->project_file_path)
    g_free (PROJECT->project_file_path);
  if (PROJECT->states_dir)
    g_free (PROJECT->states_dir);
  if (PROJECT->exports_dir)
    g_free (PROJECT->exports_dir);

  PROJECT->dir = g_strdup (dir);
  PROJECT->project_file_path =
    g_build_filename (PROJECT->dir,
                      PROJECT_FILE,
                      NULL);
  g_message ("project file path %s",
             PROJECT->project_file_path);
  PROJECT->regions_file_path =
    g_build_filename (PROJECT->dir,
                      PROJECT_REGIONS_FILE,
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
  engine_init (&self->audio_engine, 0);
  undo_manager_init (&self->undo_manager);

  self->title = g_strdup (DEFAULT_PROJECT_NAME);

  /* add master channel to mixer */
  mixer_add_channel (
    channel_create (CT_MASTER, "Master"));

  /* create chord track */
  self->chord_track = chord_track_default ();
  self->chord_track_id = self->chord_track->id;

  self->loaded = 1;

  snap_grid_init (&PROJECT->snap_grid_timeline,
                  NOTE_LENGTH_1_1);
  quantize_init (&PROJECT->quantize_timeline,
                 NOTE_LENGTH_1_1);
  snap_grid_init (&PROJECT->snap_grid_midi,
                  NOTE_LENGTH_1_8);
  quantize_init (&PROJECT->quantize_midi,
                NOTE_LENGTH_1_8);
  clip_editor_init (&PROJECT->clip_editor);
  snap_grid_update_snap_points (&PROJECT->snap_grid_timeline);
  snap_grid_update_snap_points (&PROJECT->snap_grid_midi);
  quantize_update_snap_points (&PROJECT->quantize_timeline);
  quantize_update_snap_points (&PROJECT->quantize_midi);
  tracklist_init (&PROJECT->tracklist, 0);
}

/** sl = singular lowercase */
#define INIT_LOADED(sl) \
  static void \
  init_loaded_##sl##s () \
  { \
    for (int i = 0; i < PROJECT->num_##sl##s; i++) \
      sl##_init_loaded (project_get_##sl (i)); \
  }

INIT_LOADED (port)
INIT_LOADED (region)
INIT_LOADED (channel)
INIT_LOADED (plugin)
INIT_LOADED (track)
INIT_LOADED (automation_point)
INIT_LOADED (automation_curve)
INIT_LOADED (midi_note)
INIT_LOADED (chord)
INIT_LOADED (automatable)
INIT_LOADED (automation_track)
INIT_LOADED (automation_lane)

static int
load (char * filename)
{
  g_assert (filename);
  char * dir = io_get_dir (filename);
  update_paths (dir);

  gchar * yaml;
  GError *err = NULL;

  g_file_get_contents (
    PROJECT->project_file_path,
    &yaml,
    NULL,
    &err);
  if (err != NULL)
    {
      // Report error to user, and free error
      char * str =
        g_strdup_printf (
          "Unable to read file: %s",
          err->message);
      ui_show_error_message (
        MAIN_WINDOW,
        str);
      g_free (str);
      g_error_free (err);
      RETURN_ERROR
    }

  Project * prj = project_deserialize (yaml);

  if (prj == NULL)
    {
      ui_show_error_message (
        MAIN_WINDOW,
        "Failed to load project. Please check the "
        "logs for more information.");
      RETURN_ERROR;
    }

  int loading_while_running = PROJECT->loaded;
  if (loading_while_running)
    {
      tear_down (PROJECT);
      PROJECT = prj;

      MainWindowWidget * mww = MAIN_WINDOW;

      g_message ("recreating main window...");
      MAIN_WINDOW =
        main_window_widget_new (zrythm_app);

      g_message ("destroying previous main "
                 "window...");
      gtk_widget_destroy (GTK_WIDGET (mww));
    }

  g_message ("initing loaded structures");
  PROJECT = prj;
  update_paths (dir);
  undo_manager_init (&PROJECT->undo_manager);
  init_loaded_ports ();
  engine_init (AUDIO_ENGINE, 1);
  init_loaded_regions ();
  init_loaded_channels ();
  init_loaded_plugins ();
  init_loaded_tracks ();
  init_loaded_midi_notes ();
  init_loaded_automation_points ();
  init_loaded_automation_curves ();
  init_loaded_chords ();
  init_loaded_automatables ();
  init_loaded_automation_tracks ();
  init_loaded_automation_lanes ();
  /*mixer_load_plugins ();*/

  char * filepath_noext = g_path_get_basename (dir);
  PROJECT->title = filepath_noext;

  g_free (dir);

  PROJECT->filename = filename;

  tracklist_init (&PROJECT->tracklist, 1);

  snap_grid_update_snap_points (
    &PROJECT->snap_grid_timeline);
  snap_grid_update_snap_points (
    &PROJECT->snap_grid_midi);
  quantize_update_snap_points (
    &PROJECT->quantize_timeline);
  quantize_update_snap_points (
    &PROJECT->quantize_midi);

  PROJECT->loaded = 1;

  /* mimic behavior when starting the app */
  if (loading_while_running)
    {
      events_init (&ZRYTHM->events);
      main_window_widget_refresh (MAIN_WINDOW);

      AUDIO_ENGINE->run = 1;
    }

  RETURN_OK;
}

#undef INIT_LOADED(sl)

/**
 * If project has a filename set, it loads that. Otherwise
 * it loads the default project.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
int
project_load (char * filename)
{
  if (filename)
    return load (filename);
  else
    create_default (PROJECT);
  return 0;
}

/**
 * Sets if the project has range and updates UI.
 */
void
project_set_has_range (int has_range)
{
  PROJECT->has_range = has_range;

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED, NULL);
}

int
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
                    g_strdup_printf (
                      "%s_%d",
                      channel->track->name,
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
  GError *err = NULL;
  g_file_set_contents (
    PROJECT->project_file_path,
    yaml,
    -1,
    &err);
  if (err != NULL)
    {
      // Report error to user, and free error
      g_warning ("Unable to read file: %s",
                 err->message);
      g_error_free (err);
      RETURN_ERROR;
    }

  zrythm_add_to_recent_projects (
    ZRYTHM,
    PROJECT->project_file_path);
  PROJECT->title = g_path_get_basename (dir);

  ui_show_notification ("Project saved.");

  RETURN_OK;
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

#define PROJECT_ADD_X(camelcase, lowercase) \
  void \
  project_add_##lowercase (camelcase * x) \
  { \
    x->id = \
      get_next_available_id ( \
        (void **) PROJECT->lowercase##s, \
        PROJECT->num_##lowercase##s); \
    PROJECT->lowercase##s[x->id] = x; \
    PROJECT->num_##lowercase##s++; \
  }
#define PROJECT_GET_X(camelcase, lowercase) \
  camelcase * \
  project_get_##lowercase (int id) \
  { \
    if (id < 0) \
      return NULL; \
    \
    return PROJECT->lowercase##s[id]; \
  }


PROJECT_ADD_X (Region, region)
PROJECT_GET_X (Region, region)
PROJECT_ADD_X (Track, track)
PROJECT_GET_X (Track, track)
PROJECT_ADD_X (Channel, channel)
PROJECT_GET_X (Channel, channel)
PROJECT_ADD_X (Plugin, plugin)
PROJECT_GET_X (Plugin, plugin)
PROJECT_ADD_X (AutomationPoint, automation_point)
PROJECT_GET_X (AutomationPoint, automation_point)
PROJECT_ADD_X (AutomationCurve, automation_curve)
PROJECT_GET_X (AutomationCurve, automation_curve)
PROJECT_ADD_X (MidiNote, midi_note)
PROJECT_GET_X (MidiNote, midi_note)
PROJECT_ADD_X (Port, port)
PROJECT_GET_X (Port, port)
PROJECT_ADD_X (Chord, chord)
PROJECT_GET_X (Chord, chord)
PROJECT_ADD_X (Automatable, automatable)
PROJECT_GET_X (Automatable, automatable)
PROJECT_ADD_X (AutomationTrack, automation_track)
PROJECT_GET_X (AutomationTrack, automation_track)
PROJECT_ADD_X (AutomationLane, automation_lane)
PROJECT_GET_X (AutomationLane, automation_lane)

#undef PROJECT_ADD_X
#undef PROJECT_GET_X

SERIALIZE_SRC (Project, project)
DESERIALIZE_SRC (Project, project)
PRINT_YAML_SRC (Project, project)
