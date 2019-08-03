/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/create_tracks_action.h"
#include "audio/audio_region.h"
#include "audio/channel.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/tracklist.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

/**
 * @param add_to_project Used when the track to
 *   create is meant to be used in the project (ie
 *   not one of the tracks in CreateTracksAction.
 */
static int
create (
  CreateTracksAction * self,
  int                  idx,
  int                  add_to_project)
{
  Track * track;

  if (self->is_empty)
    {
      char * tmp =
        track_stringize_type (self->type);
      char * label;
      label =
        g_strdup_printf (_("%s Track"), tmp);
      g_free (tmp);

      track =
        track_new (
          self->type, label);
      if (add_to_project)
        tracklist_insert_track (
          TRACKLIST,
          track,
          self->pos + idx,
          F_NO_PUBLISH_EVENTS,
          F_NO_RECALC_GRAPH);
    }
  else
    {
      track =
        track_new (self->type, self->pl_descr.name);
      track->pos = self->pos + idx;

      Plugin * pl=
        plugin_new_from_descr (
          &self->pl_descr);
      pl->slot = 0;
      pl->track = track;
      pl->track_pos = track->pos;

      if (plugin_instantiate (pl) < 0)
        {
          char * message =
            g_strdup_printf (
              _("Error instantiating plugin %s. "
                "Please see log for details."),
              pl->descr->name);

          if (MAIN_WINDOW)
            ui_show_error_message (
              GTK_WINDOW (MAIN_WINDOW),
              message);
          g_free (message);
          plugin_free (pl);
          return -1;
        }

      if (add_to_project)
        tracklist_insert_track (
          TRACKLIST,
          track,
          track->pos,
          F_NO_PUBLISH_EVENTS,
          F_NO_RECALC_GRAPH);

      if (track->channel)
        {
          channel_add_plugin (
            track->channel, pl->slot, pl,
            1, 1, F_NO_RECALC_GRAPH);
          g_warn_if_fail (pl->track == track);
        }

      if (self->type == TRACK_TYPE_AUDIO)
        {
          /* create an audio region & add to track */
          Position start_pos;
          position_set_to_pos (
            &start_pos, PLAYHEAD);
          AudioRegion * ar =
            audio_region_new (
              self->file_descr.absolute_path,
              &start_pos, 1);
          track_add_region (
            track, ar, 0, F_GEN_NAME,
            F_GEN_WIDGET);
        }

      if (g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate") &&
          add_to_project)
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  return 0;
}

UndoableAction *
create_tracks_action_new (
  TrackType          type,
  PluginDescriptor * pl_descr,
  FileDescriptor *   file_descr,
  int                pos,
  int                num_tracks)
{
	CreateTracksAction * self =
    calloc (1, sizeof (
    	CreateTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_CREATE_TRACKS;
  if (pl_descr)
    plugin_clone_descr (pl_descr, &self->pl_descr);
  else if (file_descr)
    file_manager_clone_descr (
      file_descr, &self->file_descr);
  else
    self->is_empty = 1;
  self->pos = pos;
  self->type = type;
  self->num_tracks = num_tracks;
  for (int i = 0; i < num_tracks; i++)
    {
      /* create clones for reference */
      create (self, i, 0);
    }

  return ua;
}

int
create_tracks_action_do (
	CreateTracksAction * self)
{
  int ret;
  for (int i = 0; i < self->num_tracks; i++)
    {
      ret = create (self, i, 1);
      g_return_val_if_fail (!ret, -1);
    }

  EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

  mixer_recalc_graph (MIXER);

  return 0;
}

/**
 * Deletes the track.
 */
int
create_tracks_action_undo (
	CreateTracksAction * self)
{
  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track =
        TRACKLIST->tracks[self->pos + i];
      g_return_val_if_fail (track, -1);

      tracklist_remove_track (
        TRACKLIST,
        track,
        F_REMOVE_PL,
        F_FREE,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);

  mixer_recalc_graph (MIXER);

  return 0;
}

char *
create_tracks_action_stringize (
	CreateTracksAction * self)
{
  if (self->num_tracks == 1)
    return g_strdup_printf (
      _("Create %s Track"),
      track_stringize_type (
        self->type));
  else
    return g_strdup_printf (
      _("Create %d %s Tracks"),
      self->num_tracks,
      track_stringize_type (
        self->type));
}

void
create_tracks_action_free (
	CreateTracksAction * self)
{
  free (self);
}
