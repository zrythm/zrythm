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
#include "audio/marker_track.h"
#include "audio/midi_note.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/header_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/lv2_plugin.h"
#include "utils/arrays.h"
#include "utils/general.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/smf.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

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

  track_free (P_CHORD_TRACK);

  free (self);
}

static void
update_paths (const char * dir)
{
  if (PROJECT->dir)
    g_free (PROJECT->dir);
  if (PROJECT->project_file_path)
    g_free (PROJECT->project_file_path);
  if (PROJECT->states_dir)
    g_free (PROJECT->states_dir);
  if (PROJECT->exports_dir)
    g_free (PROJECT->exports_dir);
  if (PROJECT->audio_dir)
    g_free (PROJECT->audio_dir);

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
  PROJECT->audio_dir =
    g_build_filename (PROJECT->dir,
                      PROJECT_AUDIO_DIR,
                      NULL);
  g_message ("updated paths %s", PROJECT->dir);
}

/**
 * Checks that everything is okay with the project.
 */
void
project_sanity_check (Project * self)
{
  /* TODO */
}

void
create_default (Project * self)
{
  engine_init (&self->audio_engine, 0);
  undo_manager_init (&self->undo_manager);

  self->title = g_strdup (DEFAULT_PROJECT_NAME);

  /* init pinned tracks */
  Track * track =
    chord_track_new ();
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  TRACKLIST->chord_track = track;
  track =
    marker_track_default ();
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track->pinned = 1;
  TRACKLIST->marker_track = track;

  /* add master channel to mixer and tracklist */
  track =
    track_new (TRACK_TYPE_MASTER, _("Master"));
  MIXER->master = track;
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);

  /* create untitled project */
  char * untitled_project = _("Untitled Project");
  char * dir =
    g_strdup_printf ("%s%s%s",
                     ZRYTHM->projects_dir,
                     G_DIR_SEPARATOR_S,
                     untitled_project);
  update_paths (dir);
  int i = 1;
  while (io_file_exists (dir) &&
         PROJECT->project_file_path &&
         io_file_exists (PROJECT->project_file_path))
    {
      g_free (dir);
      dir =
        g_strdup_printf ("%s%s%s (%d)",
                         ZRYTHM->projects_dir,
                         G_DIR_SEPARATOR_S,
                         untitled_project,
                         i++);
      update_paths (dir);
    }
  io_mkdir (dir);
  char * filepath_noext = g_path_get_basename (dir);
  PROJECT->title = filepath_noext;

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

  header_notebook_widget_set_subtitle (
    MW_HEADER_NOTEBOOK,
    PROJECT->title);
}

static int
load (char * filename)
{
  g_warn_if_fail (filename);
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
  /*init_loaded_ports ();*/
  engine_init (AUDIO_ENGINE, 1);
  /*init_loaded_regions ();*/
  /*init_loaded_plugins ();*/
  /*init_loaded_tracks ();*/
  /*init_loaded_midi_notes ();*/
  /*init_loaded_automation_points ();*/
  /*init_loaded_automation_curves ();*/
  /*init_loaded_chords ();*/
  /*init_loaded_automatables ();*/
  /*init_loaded_automation_tracks ();*/
  /*init_loaded_automation_lanes ();*/
  timeline_selections_init_loaded (
    &PROJECT->timeline_selections);
  midi_arranger_selections_init_loaded (
    &PROJECT->midi_arranger_selections);
  tracklist_selections_init_loaded (
    &PROJECT->tracklist_selections);
  g_message ("loaded structures");

  char * filepath_noext = g_path_get_basename (dir);
  PROJECT->title = filepath_noext;

  g_free (dir);

  tracklist_init_loaded (&PROJECT->tracklist);

  snap_grid_update_snap_points (
    &PROJECT->snap_grid_timeline);
  snap_grid_update_snap_points (
    &PROJECT->snap_grid_midi);
  quantize_update_snap_points (
    &PROJECT->quantize_timeline);
  quantize_update_snap_points (
    &PROJECT->quantize_midi);

  /* sanity check */
  project_sanity_check (PROJECT);

  PROJECT->loaded = 1;
  g_message ("project loaded");

  /* mimic behavior when starting the app */
  if (loading_while_running)
    {
      if (ZRYTHM->event_queue)
        g_async_queue_unref (ZRYTHM->event_queue);
      ZRYTHM->event_queue =
        events_init ();
      main_window_widget_refresh (MAIN_WINDOW);

      g_atomic_int_set (&AUDIO_ENGINE->run, 1);
    }

  header_notebook_widget_set_subtitle (
    MW_HEADER_NOTEBOOK,
    PROJECT->title);

  RETURN_OK;
}

/**
 * If project has a filename set, it loads that.
 * Otherwise it loads the default project.
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

  /*smf_save_regions ();*/

  /* write plugin states */
  Track * track;
  Channel * ch;
  Plugin * pl;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        continue;

      ch = track->channel;

      for (int j = 0; j < STRIP_SIZE; j++)
        {
          pl = ch->plugins[j];

          if (!pl)
            continue;

          if (pl->descr->protocol == PROT_LV2)
            {
              char * tmp =
                g_strdup_printf (
                  "%s_%d",
                  ch->track->name,
                  j);
              char * state_dir_plugin =
                g_build_filename (
                  PROJECT->states_dir,
                  tmp,
                  NULL);
              g_free (tmp);

              Lv2Plugin * lv2_plugin =
                (Lv2Plugin *)
                pl->lv2;
              lv2_plugin_save_state_to_file (
                lv2_plugin,
                state_dir_plugin);
              g_free (state_dir_plugin);
            }
        }
    }

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

  ui_show_notification (_("Project saved."));

  header_notebook_widget_set_subtitle (
    MW_HEADER_NOTEBOOK,
    PROJECT->title);

  RETURN_OK;
}

SERIALIZE_SRC (Project, project)
DESERIALIZE_SRC (Project, project)
PRINT_YAML_SRC (Project, project)
