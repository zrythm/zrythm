/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/delete_tracks_action.h"
#include "audio/router.h"
#include "audio/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
delete_tracks_action_init_loaded (
  DeleteTracksAction * self)
{
  tracklist_selections_init_loaded (
    self->tls);
}

UndoableAction *
delete_tracks_action_new (
  TracklistSelections * tls)
{
  DeleteTracksAction * self =
    calloc (1, sizeof (
      DeleteTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UA_DELETE_TRACKS;

  self->tls = tracklist_selections_clone (tls);
  tracklist_selections_sort (self->tls);

  return ua;
}

int
delete_tracks_action_do (
  DeleteTracksAction * self)
{
  Track * track;

  for (int i = self->tls->num_tracks - 1; i >= 0; i--)
    {
      /* get track from pos */
      track =
        TRACKLIST->tracks[
          self->tls->tracks[i]->pos];
      g_return_val_if_fail (track, -1);

      /* remove it */
      g_message (
        "removing track %s...", track->name);
      tracklist_remove_track (
        TRACKLIST, track, F_REMOVE_PL,
        F_FREE, F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);
  EVENTS_PUSH (ET_CLIP_EDITOR_REGION_CHANGED, NULL);

  router_recalc_graph ((ROUTER));

  return 0;
}

/**
 * Undo deletion, create tracks.
 */
int
delete_tracks_action_undo (
  DeleteTracksAction * self)
{
  Track * track;
  for (int i = 0; i < self->tls->num_tracks; i++)
    {
      /* clone the clone */
      track =
        track_clone (self->tls->tracks[i], false);

      /* insert it to the tracklist at its original
       * pos */
      tracklist_insert_track (
        TRACKLIST, track, track->pos,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);

      if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          if (track->channel->instrument->visible)
            {
              EVENTS_PUSH (
                ET_PLUGIN_VISIBILITY_CHANGED,
                track->channel->instrument);
            }
        }
    }

  EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

  /* recalculate graph */
  router_recalc_graph ((ROUTER));

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
  object_free_w_func_and_null (
    tracklist_selections_free, self->tls);

  object_zero_and_free (self);
}

