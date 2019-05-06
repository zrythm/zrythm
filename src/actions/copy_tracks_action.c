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

#include "actions/copy_tracks_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

UndoableAction *
copy_tracks_action_new (
  TracklistSelections * tls,
  int                   pos)
{
  CopyTracksAction * self =
    calloc (1, sizeof (
                 CopyTracksAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_COPY_TRACKS;

  self->tls = tracklist_selections_clone (tls);
  self->pos = pos;
  return ua;
}

int
copy_tracks_action_do (
  CopyTracksAction * self)
{
  Track * track, * orig_track;

	for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* get the clone */
      orig_track = self->tls->tracks[i];

      /* clone the clone */
      track = track_clone (orig_track);

      /* add to project to get a unique ID */
      project_add_track (track);

      /* add to tracklist at given pos */
      tracklist_insert_track (
        TRACKLIST,
        track,
        self->pos + i,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      /* select the new clone */
      /* TODO */

      /* remember the new clone's id */
      orig_track->id = track->id;
    }

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
copy_tracks_action_undo (
  CopyTracksAction * self)
{
  Track * track;

  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* get the track from the clone ID */
      track =
        project_get_track (
          self->tls->tracks[i]->id);

      /* remove it */
      tracklist_remove_track (
        TRACKLIST,
        track,
        F_FREE,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
copy_tracks_action_stringize (
  CopyTracksAction * self)
{
  /* FIXME */
  return g_strdup ("Copy Tracks");
}

void
copy_tracks_action_free (
  CopyTracksAction * self)
{
  tracklist_selections_free (self->tls);

  free (self);
}
