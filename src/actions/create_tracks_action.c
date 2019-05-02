/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

  return ua;
}

static int
create (
  CreateTracksAction * self,
  TrackType            type,
  int                  idx)
{
  Track * track;

  if (self->is_empty)
    {
      char * tmp = track_stringize_type (type);
      char * label =
        g_strdup_printf (_("%s Track"), tmp);
      g_free (tmp);

      track =
        track_new (type,
                   label);
      tracklist_insert_track (
        TRACKLIST,
        track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
      if (track->channel)
        mixer_add_channel (
          MIXER, track->channel, F_NO_RECALC_GRAPH);
      tracklist_insert_track (
        TRACKLIST,
        track,
        self->pos + idx,
        F_NO_PUBLISH_EVENTS);
    }
  else
    {
      Plugin * pl=
        plugin_new_from_descr (
          &self->pl_descr,
          F_ADD_TO_PROJ);

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

      track =
        track_new (type, self->pl_descr.name);
      tracklist_insert_track (
        TRACKLIST,
        track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
      if (track->channel)
      channel_add_plugin (
        track->channel, 0, pl, 1, 1, 1);
      mixer_add_channel (
        MIXER, track->channel, F_NO_RECALC_GRAPH);

      if (type == TRACK_TYPE_AUDIO)
        {
          /* create an audio region & add to track */
          Position start_pos;
          position_set_to_pos (&start_pos,
                               &PLAYHEAD);
          AudioRegion * ar =
            audio_region_new (
              track,
              self->file_descr.absolute_path,
              &start_pos,
              F_ADD_TO_PROJ);
          track_add_region (
            track, ar);
        }

      if (g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  mixer_recalc_graph (MIXER);

  self->track_ids[idx] = track->id;

  return 0;
}

int
create_tracks_action_do (
	CreateTracksAction * self)
{
  int ret;
  for (int i = 0; i < self->num_tracks; i++)
    {
      ret = create (self, self->type, i);
      g_return_val_if_fail (!ret, -1);
    }

  EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

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
        project_get_track (self->track_ids[i]);
      g_return_val_if_fail (track, -1);

      mixer_remove_channel (
        MIXER,
        track->channel,
        F_NO_PUBLISH_EVENTS);

      tracklist_remove_track (
        TRACKLIST,
        track,
        F_FREE,
        F_NO_PUBLISH_EVENTS);
    }

  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);

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
