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

#include "actions/delete_tracks_action.h"
#include "audio/mixer.h"
#include "audio/tracklist.h"
#include "gui/backend/tracklist_selections.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

UndoableAction *
delete_tracks_action_new (
  TracklistSelections * tls)
{
	DeleteTracksAction * self =
    calloc (1, sizeof (
    	DeleteTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_DELETE_TRACKS;

  self->tls = tracklist_selections_clone (tls);

  return ua;
}

int
delete_tracks_action_do (
	DeleteTracksAction * self)
{
  Track * track;

  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      track =
        project_get_track (
          self->tls->track_ids[i]);
      g_return_val_if_fail (track, -1);

      tracklist_remove_track (
        TRACKLIST,
        track,
        F_FREE,
        F_NO_PUBLISH_EVENTS,
        F_RECALC_GRAPH);
    }

  return 0;
}

/**
 * Undo deletion, create tracks.
 */
int
delete_tracks_action_undo (
	DeleteTracksAction * self)
{
  Track * track, * orig_track;
  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* get the clone */
      orig_track = self->tls->tracks[i];

      /* clone the clone */
      track = track_clone (orig_track);

      /* add to project to get a new ID */
      project_add_track (track);

      /* connect the track */
      tracklist_insert_track (
        TRACKLIST,
        track,
        track->pos,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      /* update the ID */
      orig_track->id = track->id;
    }

  /* recalculate graph */
  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

  return 0;
}

char *
delete_tracks_action_stringize (
	DeleteTracksAction * self)
{
  if (self->tls->num_tracks == 1)
    return g_strdup (_("Delete Track"));
  else
    return g_strdup_printf (
      _("Delete %d Tracks"),
      self->tls->num_tracks);
}

void
delete_tracks_action_free (
	DeleteTracksAction * self)
{
  tracklist_selections_free (self->tls);
  free (self);
}

