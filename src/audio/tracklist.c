/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/router.h"
#include "audio/tracklist.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

/**
 * Initializes the tracklist when loading a project.
 */
void
tracklist_init_loaded (
  Tracklist * self)
{
  g_message ("initializing loaded Tracklist...");
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];

      if (track->type == TRACK_TYPE_CHORD)
        self->chord_track = track;
      else if (track->type == TRACK_TYPE_MARKER)
        self->marker_track = track;
      else if (track->type == TRACK_TYPE_MASTER)
        self->master_track = track;
      else if (track->type == TRACK_TYPE_TEMPO)
        self->tempo_track = track;

      track_init_loaded (track, true);
    }
}

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (
  Tracklist * self,
  Track **    visible_tracks,
  int *       num_visible)
{
  *num_visible = 0;
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (track->visible)
        {
          visible_tracks[*num_visible++] = track;
        }
    }
}

/**
 * Returns the number of visible Tracks between
 * src and dest (negative if dest is before src).
 */
int
tracklist_get_visible_track_diff (
  Tracklist * self,
  const Track *     src,
  const Track *     dest)
{
  g_return_val_if_fail (
    src && dest, 0);

  int count = 0;
  if (src->pos < dest->pos)
    {
      for (int i = src->pos; i < dest->pos; i++)
        {
          Track * track = self->tracks[i];
          if (track->visible)
            {
              count++;
            }
        }
    }
  else if (src->pos > dest->pos)
    {
      for (int i = dest->pos; i < src->pos; i++)
        {
          Track * track = self->tracks[i];
          if (track->visible)
            {
              count--;
            }
        }
    }

  return count;
}

int
tracklist_contains_master_track (
  Tracklist * self)
{
  for (int i = 0; self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (track->type == TRACK_TYPE_MASTER)
        return 1;
    }
  return 0;
}

int
tracklist_contains_chord_track (
  Tracklist * self)
{
  for (int i = 0; self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        return 1;
    }
  return 0;
}

void
tracklist_print_tracks (
  Tracklist * self)
{
  g_message ("----- tracklist tracks ------");
  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      g_message ("[idx %d] %s (pos %d)",
                 i, track->name,
                 track->pos);
    }
  g_message ("------ end ------");
}

/**
 * Adds given track to given spot in tracklist.
 *
 * @param publish_events Publish UI events.
 * @param recalc_graph Recalculate routing graph.
 */
void
tracklist_insert_track (
  Tracklist * self,
  Track *     track,
  int         pos,
  int         publish_events,
  int         recalc_graph)
{
  g_message (
    "inserting %s at %d...",
    track->name, pos);

  track_set_name (track, track->name, 0);

  /* if adding at the end, append it, otherwise
   * insert it */
  if (pos == self->num_tracks)
    {
      array_append (
        self->tracks, self->num_tracks, track);
    }
  else
    {
      array_insert (
        self->tracks, self->num_tracks,
        pos, track);
    }

  track_set_pos (track, pos);

  /* make the track the only selected track */
  tracklist_selections_select_single (
    TRACKLIST_SELECTIONS, track);

  /* move other tracks */
  for (int i = 0;
       i < self->num_tracks; i++)
    {
      if (i == pos)
        continue;

      track_set_pos (self->tracks[i], i);
    }

  track_set_is_project (track, true);

  /* this is needed again since "set_is_project"
   * made some ports from non-project to project
   * and they weren't considered before */
  track_set_pos (track, pos);

  if (track->channel)
    {
      channel_connect (track->channel);
    }

  /* verify */
  track_verify_identifiers (track);

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  if (publish_events)
    {
      EVENTS_PUSH (ET_TRACK_ADDED, track);
    }

  g_message ("%s: done", __func__);
}

ChordTrack *
tracklist_get_chord_track (
  const Tracklist * self)
{
  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        {
          return (ChordTrack *) track;
        }
    }
  g_warn_if_reached ();
  return NULL;
}

/**
 * Returns the Track matching the given name, if
 * any.
 */
Track *
tracklist_find_track_by_name (
  Tracklist *  self,
  const char * name)
{
  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      if (string_is_equal (name, track->name, 0))
        return track;
    }
  return NULL;
}

void
tracklist_append_track (
  Tracklist * self,
  Track *     track,
  int         publish_events,
  int         recalc_graph)
{
  tracklist_insert_track (
    self, track, self->num_tracks,
    publish_events, recalc_graph);
}

int
tracklist_get_track_pos (
  Tracklist * self,
  Track *     track)
{
  return
    array_index_of (
      (void **) self->tracks,
      self->num_tracks,
      (void *) track);
}

/**
 * Returns the index of the last Track.
 *
 * @param pinned_only Only consider pinned Track's.
 * @param visible_only Only consider visible
 *   Track's.
 */
int
tracklist_get_last_pos (
  Tracklist * self,
  const int   pinned_only,
  const int   visible_only)
{
  Track * tr;
  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      tr = self->tracks[i];
      if (((pinned_only && tr->pinned) ||
           !pinned_only) &&
          ((visible_only && tr->visible) ||
           !visible_only))
        {
          return i;
        }
    }

  /* no track with given options found,
   * select the last */
  return self->num_tracks - 1;
}

/**
 * Returns the last Track.
 *
 * @param pinned_only Only consider pinned Track's.
 * @param visible_only Only consider visible
 *   Track's.
 */
Track*
tracklist_get_last_track (
  Tracklist * self,
  const int   pinned_only,
  const int   visible_only)
{
  int idx =
    tracklist_get_last_pos (
      self, pinned_only, visible_only);
  g_return_val_if_fail (
    idx >= 0 && idx < self->num_tracks, NULL);
  Track * tr =
    self->tracks[idx];

  return tr;
}

/**
 * Returns the Track after delta visible Track's.
 *
 * Negative delta searches backwards.
 *
 * This function searches tracks only in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_visible_track_after_delta (
  Tracklist * self,
  Track *     track,
  int         delta)
{
  if (delta > 0)
    {
      Track * vis_track = track;
      while (delta > 0)
        {
          vis_track =
            tracklist_get_next_visible_track (
              self, vis_track);

          if (!vis_track)
            return NULL;

          delta--;
        }
      return vis_track;
    }
  else if (delta < 0)
    {
      Track * vis_track = track;
      while (delta < 0)
        {
          vis_track =
            tracklist_get_prev_visible_track (
              self, vis_track);

          if (!vis_track)
            return NULL;

          delta++;
        }
      return vis_track;
    }
  else
    return track;
}

/**
 * Returns the first visible Track.
 *
 * @param pinned 1 to check the pinned tracklist,
 *   0 to check the non-pinned tracklist.
 */
Track *
tracklist_get_first_visible_track (
  Tracklist * self,
  const int   pinned)
{
  Track * tr;
  for (int i = 0; i < self->num_tracks; i++)
    {
      tr = self->tracks[i];
      if (tr->visible && tr->pinned == pinned)
        {
          return self->tracks[i];
        }
    }
  g_warn_if_reached ();
  return NULL;
}

/**
 * Returns the previous visible Track.
 */
Track *
tracklist_get_prev_visible_track (
  Tracklist * self,
  Track *     track)
{
  Track * tr;
  for (int i =
       tracklist_get_track_pos (self, track) - 1;
       i >= 0; i--)
    {
      tr = self->tracks[i];
      if (tr->visible)
        {
          g_warn_if_fail (tr != track);
          return tr;
        }
    }
  return NULL;
}

/**
 * Pins or unpins the Track.
 */
void
tracklist_set_track_pinned (
  Tracklist * self,
  Track *     track,
  const int   pinned,
  int         publish_events,
  int         recalc_graph)
{
  if (track->pinned == pinned)
    return;

  int last_pinned_pos =
    tracklist_get_last_pos (
      self, 1, 0);
  if (pinned)
    {
      /* move track to last pinned pos + 1 */
      tracklist_move_track (
        self, track, last_pinned_pos + 1,
        publish_events,
        recalc_graph);
      track->pinned = 1;
      track->pos_before_pinned =
        track->pos;
    }
  else
    {
      /* move track to previous pos */
      g_return_if_fail (
        track->pos_before_pinned >= 0);
      int pos_to_move_to;
      if (track->pos_before_pinned <=
            last_pinned_pos)
        pos_to_move_to = last_pinned_pos + 1;
      else
        pos_to_move_to = track->pos_before_pinned;
      track->pinned = 0;
      tracklist_move_track (
        self, track, pos_to_move_to,
        publish_events, recalc_graph);
      track->pos_before_pinned = -1;
    }
}

/**
 * Returns the next visible Track in the same
 * Tracklist.
 */
Track *
tracklist_get_next_visible_track (
  Tracklist * self,
  Track *     track)
{
  Track * tr;
  for (int i =
       tracklist_get_track_pos (self, track) + 1;
       i < self->num_tracks; i++)
    {
      tr = self->tracks[i];
      if (tr->visible)
        {
          g_warn_if_fail (tr != track);
          return tr;
        }
    }
  return NULL;
}

/**
 * Removes a track from the Tracklist and the
 * TracklistSelections.
 *
 * Also disconnects the channel.
 *
 * @param rm_pl Remove plugins or not.
 * @param free Free the track or not (free later).
 * @param publish_events Push a track deleted event
 *   to the UI.
 * @param recalc_graph Recalculate the mixer graph.
 */
void
tracklist_remove_track (
  Tracklist * self,
  Track *     track,
  bool        rm_pl,
  bool        free_track,
  bool        publish_events,
  bool        recalc_graph)
{
  g_message (
    "%s: removing %s...", __func__, track->name);

  Track * prev_visible =
    tracklist_get_prev_visible_track (
      TRACKLIST, track);
  Track * next_visible =
    tracklist_get_next_visible_track (
      TRACKLIST, track);

  /* clear the editor region if it exists and
   * belongs to this track */
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (region &&
      arranger_object_get_track (
        (ArrangerObject *) region) == track)
    {
      clip_editor_set_region (
        CLIP_EDITOR, NULL, publish_events);
    }

  /* remove/deselect all objects */
  track_clear (track);

  int idx =
    array_index_of (
      self->tracks, self->num_tracks, track);
  g_warn_if_fail (
    track->pos == idx);

  track_disconnect (
    track, rm_pl, F_NO_RECALC_GRAPH);

  tracklist_selections_remove_track (
    TRACKLIST_SELECTIONS, track, publish_events);
  array_delete (
    self->tracks, self->num_tracks, track);

  /* if it was the only track selected, select
   * the next one */
  if (TRACKLIST_SELECTIONS->num_tracks == 0 &&
      (prev_visible || next_visible))
    {
      tracklist_selections_add_track (
        TRACKLIST_SELECTIONS,
        next_visible ? next_visible : prev_visible,
        publish_events);
    }

  track_set_pos (track, -1);

  track_set_is_project (track, false);

  /* move all other tracks */
  for (int i = 0; i < self->num_tracks; i++)
    {
      track_set_pos (self->tracks[i], i);
    }

  if (free_track)
    {
      free_later (track, track_free);
    }

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  if (publish_events)
    {
      EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);
    }

  g_message ("%s: done", __func__);
}

static void
swap_tracks (
  Tracklist * self,
  int         src,
  int         dest)
{
  Track * src_track = self->tracks[src];
  Track * dest_track = self->tracks[dest];

  /* move src somewhere temporarily */
  self->tracks[src] = NULL;
  self->tracks[self->num_tracks] = src_track;
  track_set_pos (src_track, self->num_tracks);

  /* move dest to src */
  self->tracks[src] = dest_track;
  self->tracks[dest] = NULL;
  track_set_pos (dest_track, src);

  /* move src from temp pos to dest */
  self->tracks[dest] = src_track;
  self->tracks[self->num_tracks] = NULL;
  track_set_pos (src_track, dest);
}

/**
 * Moves a track from its current position to the
 * position given by \p pos.
 *
 * @param publish_events Push UI update events or
 *   not.
 * @param recalc_graph Recalculate routing graph.
 */
void
tracklist_move_track (
  Tracklist * self,
  Track *     track,
  int         pos,
  int         publish_events,
  int         recalc_graph)
{
  g_message (
    "%s: %s from %d to %d",
    __func__, track->name, track->pos, pos);
  /*int prev_pos = track->pos;*/
  bool move_higher = pos < track->pos;

  Track * prev_visible =
    tracklist_get_prev_visible_track (
      TRACKLIST, track);
  Track * next_visible =
    tracklist_get_next_visible_track (
      TRACKLIST, track);

  /* clear the editor region if it exists and
   * belongs to this track */
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (region &&
      arranger_object_get_track (
        (ArrangerObject *) region) == track)
    {
      clip_editor_set_region (
        CLIP_EDITOR, NULL, publish_events);
    }

  /* deselect all objects */
  track_unselect_all (track);

  int idx =
    array_index_of (
      self->tracks, self->num_tracks, track);
  g_warn_if_fail (
    track->pos == idx);

  tracklist_selections_remove_track (
    TRACKLIST_SELECTIONS, track, publish_events);

  /* if it was the only track selected, select
   * the next one */
  if (TRACKLIST_SELECTIONS->num_tracks == 0 &&
      (prev_visible || next_visible))
    {
      tracklist_selections_add_track (
        TRACKLIST_SELECTIONS,
        next_visible ? next_visible : prev_visible,
        publish_events);
    }

  if (move_higher)
    {
      /* move all other tracks 1 track further */
      for (int i = track->pos; i > pos; i--)
        {
          swap_tracks (self, i, i - 1);
        }
    }
  else
    {
      /* move all other tracks 1 track earlier */
      for (int i = track->pos; i < pos; i++)
        {
          swap_tracks (self, i, i + 1);
        }
    }

  /* make the track the only selected track */
  tracklist_selections_select_single (
    TRACKLIST_SELECTIONS, track);

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  if (publish_events)
    {
      EVENTS_PUSH (ET_TRACKS_MOVED, NULL);
    }

  g_message ("%s: finished moving track", __func__);
}

/**
 * Returns 1 if the track name is not taken.
 *
 * @param track_to_skip Track to skip when searching.
 */
int
tracklist_track_name_is_unique (
  Tracklist *  self,
  const char * name,
  Track *      track_to_skip)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      if (!g_strcmp0 (name, self->tracks[i]->name) &&
          self->tracks[i] != track_to_skip)
        return 0;
    }
  return 1;
}

/**
 * Returns if the tracklist has soloed tracks.
 */
int
tracklist_has_soloed (
  const Tracklist * self)
{
  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];

      if (track->channel && track_get_soloed (track))
        return 1;
    }
  return 0;
}

/**
 * Activate or deactivate all plugins.
 *
 * This is useful for exporting: deactivating and
 * reactivating a plugin will reset its state.
 */
void
tracklist_activate_all_plugins (
  Tracklist * self,
  bool        activate)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      track_activate_all_plugins (
        self->tracks[i], activate);
    }
}

/**
 * @param visible 1 for visible, 0 for invisible.
 */
int
tracklist_get_num_visible_tracks (
  Tracklist * self,
  int         visible)
{
  int ret = 0;
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];

      if (track->visible == visible)
        ret++;
    }

  return ret;
}

/**
 * Exposes each track's ports that should be
 * exposed to the backend.
 *
 * This should be called after setting up the
 * engine.
 */
void
tracklist_expose_ports_to_backend (
  Tracklist * self)
{
  g_return_if_fail (self);

  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      g_return_if_fail (track);

      if (track_type_has_channel (track->type))
        {
          Channel * ch = track_get_channel (track);
          channel_expose_ports_to_backend (ch);
        }
    }
}

Tracklist *
tracklist_new (Project * project)
{
  Tracklist * self = object_new (Tracklist);

  if (project)
    {
      project->tracklist = self;
    }

  return self;
}

void
tracklist_free (
  Tracklist * self)
{
  g_message ("%s: freeing...", __func__);

  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      Track * track = self->tracks[i];
      track_disconnect (
        track, true, F_NO_RECALC_GRAPH);
      track_set_pos (track, -1);

      track_set_is_project (track, false);
      track_free (track);
    }

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
