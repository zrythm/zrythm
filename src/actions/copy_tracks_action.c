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

#include "actions/copy_tracks_action.h"
#include "audio/router.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

void
copy_tracks_action_init_loaded (
  CopyTracksAction * self)
{
  tracklist_selections_init_loaded (
    self->tls);
}

UndoableAction *
copy_tracks_action_new (
  TracklistSelections * tls,
  int                   pos)
{
  CopyTracksAction * self =
    calloc (1, sizeof (
                 CopyTracksAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_COPY_TRACKS;

  self->tls = tracklist_selections_clone (tls);
  self->pos = pos;
  return ua;
}

int
copy_tracks_action_do (
  CopyTracksAction * self)
{
  Track * track;

	for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* create a new clone to use in the project */
      track =
        track_clone (self->tls->tracks[i], false);

      /* add to tracklist at given pos */
      tracklist_insert_track (
        TRACKLIST, track, self->pos + i,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      /* select it */
      track_select (
        track, F_SELECT, 1,
        F_NO_PUBLISH_EVENTS);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
  EVENTS_PUSH (ET_TRACK_ADDED, NULL);

  return 0;
}

int
copy_tracks_action_undo (
  CopyTracksAction * self)
{
  Track * track;

  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* get the track from the inserted pos */
      track =
        TRACKLIST->tracks[self->pos + i];
      g_return_val_if_fail (track, -1);

      /* remove it */
      tracklist_remove_track (
        TRACKLIST,
        track,
        F_REMOVE_PL,
        F_FREE,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);

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
