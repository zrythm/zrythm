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
#include "audio/router.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
move_tracks_action_init_loaded (
  MoveTracksAction * self)
{
  tracklist_selections_init_loaded (
    self->tls);
}

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
  /* sort the tracks first */
  tracklist_selections_sort (self->tls);

  /* get the project tracks */
  Track * prj_tracks[400];
  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      prj_tracks[i] =
        TRACKLIST->tracks[self->tls->tracks[i]->pos];
    }

  for (int i = self->tls->num_tracks - 1; i >= 0;
       i--)
    {
      Track * prj_track = prj_tracks[i];
      g_return_val_if_fail (prj_track, -1);

      tracklist_move_track (
        TRACKLIST, prj_track, self->pos,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      if (i == 0)
        tracklist_selections_select_single (
          TRACKLIST_SELECTIONS, prj_track);
      else
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS, prj_track, 0);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  EVENTS_PUSH (ET_TRACKS_MOVED, NULL);

  return 0;
}

int
move_tracks_action_undo (
  MoveTracksAction * self)
{
  for (int i = self->tls->num_tracks - 1; i >= 0;
       i--)
    {
      Track * prj_track =
        TRACKLIST->tracks[self->pos];
      g_return_val_if_fail (prj_track, -1);
      g_return_val_if_fail (
        self->tls->tracks[i]->pos !=
          prj_track->pos, -1);

      tracklist_move_track (
        TRACKLIST, prj_track,
        self->tls->tracks[i]->pos,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      if (i == 0)
        tracklist_selections_select_single (
          TRACKLIST_SELECTIONS, prj_track);
      else
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS, prj_track, 0);
    }

  router_recalc_graph (ROUTER, F_NOT_SOFT);

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
