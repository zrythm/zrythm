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

#include "actions/move_tracks_action.h"
#include "audio/mixer.h"
#include "audio/tracklist.h"
#include "gui/backend/tracklist_selections.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

/**
 * Move tracks to given position.
 */
UndoableAction *
move_tracks_action_new (
  TracklistSelections * tls,
  int      pos)
{
	MoveTracksAction * self =
    calloc (1, sizeof (
    	MoveTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UA_MOVE_TRACKS;

  self->pos = pos;
  self->tls = tracklist_selections_clone (tls);

  return ua;
}

int
move_tracks_action_do (
	MoveTracksAction * self)
{
  Track * track;

  /* sort the tracks first */
  tracklist_selections_sort (
    self->tls);

  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      track =
        TRACKLIST->tracks[self->tls->tracks[i]->pos];
      g_return_val_if_fail (track, -1);

      tracklist_move_track (
        TRACKLIST,
        track,
        self->pos + i,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      if (i == 0)
        tracklist_selections_select_single (
          TRACKLIST_SELECTIONS, track);
      else
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS, track);
    }

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_TRACKS_MOVED, NULL);

  return 0;
}

int
move_tracks_action_undo (
	MoveTracksAction * self)
{
  Track * track;
  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      track =
        TRACKLIST->tracks[self->tls->tracks[i]->pos];
      g_return_val_if_fail (track, -1);

      tracklist_move_track (
        TRACKLIST,
        track,
        self->tls->tracks[i]->pos,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      if (i == 0)
        tracklist_selections_select_single (
          TRACKLIST_SELECTIONS, track);
      else
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS, track);
    }

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_TRACKS_MOVED, NULL);

  return 0;
}

char *
move_tracks_action_stringize (
	MoveTracksAction * self)
{
  if (self->tls->num_tracks == 1)
    return g_strdup (
      _("Move Track"));
  else
    return g_strdup_printf (
      _("Move %d Tracks"),
      self->tls->num_tracks);
}

void
move_tracks_action_free (
	MoveTracksAction * self)
{
  tracklist_selections_free (self->tls);

  free (self);
}
