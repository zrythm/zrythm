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
#include "audio/mixer.h"
#include "audio/tracklist.h"
#include "audio/track.h"
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
#include "utils/objects.h"
#include "utils/string.h"

/**
 * Initializes the tracklist when loading a project.
 */
void
tracklist_init_loaded (
  Tracklist * self)
{
  g_message ("initializing loaded Tracklist...");
  int i;
  Track * track;
  /*Channel * chan;*/
  /*AutomationTracklist * atl;*/
  for (i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];

      if (track->type == TRACK_TYPE_CHORD)
        self->chord_track = track;
      else if (track->type == TRACK_TYPE_MARKER)
        self->marker_track = track;
      else if (track->type == TRACK_TYPE_MASTER)
        self->master_track = track;

      track_init_loaded (track);
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
    track_set_pos (self->tracks[i], i);

  if (track->channel)
    channel_connect (track->channel);

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  if (publish_events)
    EVENTS_PUSH (ET_TRACK_ADDED, track);
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
  bool        free,
  bool        publish_events,
  bool        recalc_graph)
{
  g_message ("removing %s",
             track->name);

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
      clip_editor_set_region (CLIP_EDITOR, NULL);
    }

  /* remove/deselect all objects */
  track_clear (track);

  int idx =
    array_index_of (
      self->tracks, self->num_tracks, track);
  g_warn_if_fail (
    track->pos == idx);

  if (track->channel)
    {
      channel_disconnect (
        track->channel,
        rm_pl, F_NO_RECALC_GRAPH);
    }

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

  /* move all other tracks */
  for (int i = 0; i < self->num_tracks; i++)
    {
      track_set_pos (self->tracks[i], i);
    }

  if (free)
    {
      free_later (track, track_free);
    }

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  if (publish_events)
    EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);
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
      clip_editor_set_region (CLIP_EDITOR, NULL);
    }

  /* remove/deselect all objects */
  track_clear (track);

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
      for (int i = track->pos - 1; i >= pos; i--)
        {
          track_set_pos (self->tracks[i], i + 1);
        }
    }
  else
    {
      /* move all other tracks 1 track earlier */
      for (int i = track->pos + 1; i <= pos; i++)
        {
          track_set_pos (self->tracks[i], i - 1);
        }
    }

  array_delete (
    self->tracks, self->num_tracks, track);

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

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  if (publish_events)
    EVENTS_PUSH (ET_TRACKS_MOVED, NULL);

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

      if (track->solo && track->channel)
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
