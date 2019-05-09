/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

/**
 * Initializes the tracklist when loading a project.
 */
void
tracklist_init_loaded (
  Tracklist * self)
{
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

static Track *
get_track_by_name (
  Tracklist * self,
  const char * name)
{
  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      if (g_strcmp0 (track->name, name) == 0)
        return track;
    }
  return NULL;

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

static void
set_track_name (
  Tracklist * self,
  Track * track)
{
  int count = 1;
  char * new_label = g_strdup (track->name);
  while (get_track_by_name (self, new_label))
    {
      g_free (new_label);
      /*if (DEBUGGING)*/
        /*new_label =*/
          /*g_strdup_printf ("[%d] %s %d",*/
                           /*track->id,*/
                           /*track->name,*/
                           /*count++);*/
      /*else*/
        new_label =
          g_strdup_printf ("%s %d",
                           track->name,
                           count++);
    }
  track->name = new_label;
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
  /* FIXME do it externally */
  set_track_name (self, track);

  array_insert (
    self->tracks,
    self->num_tracks,
    pos,
    track);

  track->pos = pos;

  /* move other tracks */
  for (int i = track->pos + 1;
       i < self->num_tracks; i++)
    self->tracks[i]->pos = i;

  if (track->channel)
    channel_connect (track->channel);

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  if (publish_events)
    EVENTS_PUSH (ET_TRACK_ADDED, track);
}

ChordTrack *
tracklist_get_chord_track (
  Tracklist * self)
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

void
tracklist_append_track (
  Tracklist * self,
  Track *     track,
  int         publish_events,
  int         recalc_graph)
{
  tracklist_insert_track (
    self,
    track,
    self->num_tracks,
    publish_events,
    recalc_graph);
}

int
tracklist_get_track_pos (
  Tracklist * self,
  Track *     track)
{
  return array_index_of ((void **) self->tracks,
                         self->num_tracks,
                         (void *) track);
}

int
tracklist_get_last_visible_pos (
  Tracklist * self)
{
  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      if (self->tracks[i]->visible)
        {
          return i;
        }
    }
  g_warn_if_reached ();
  return -1;
}

Track*
tracklist_get_last_visible_track (
  Tracklist * self)
{
  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      if (self->tracks[i]->visible)
        {
          return self->tracks[i];
        }
    }
  g_warn_if_reached ();
  return NULL;
}

Track *
tracklist_get_first_visible_track (
  Tracklist * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      if (self->tracks[i]->visible)
        {
          return self->tracks[i];
        }
    }
  g_warn_if_reached ();
  return NULL;
}

Track *
tracklist_get_prev_visible_track (
  Tracklist * self,
  Track * track)
{
  for (int i =
       tracklist_get_track_pos (self, track) - 1;
       i >= 0; i--)
    {
      if (self->tracks[i]->visible)
        {
          g_warn_if_fail (self->tracks[i] != track);
          return self->tracks[i];
        }
    }
  return NULL;
}

Track *
tracklist_get_next_visible_track (
  Tracklist * self,
  Track * track)
{
  for (int i =
       tracklist_get_track_pos (self, track) + 1;
       i < self->num_tracks; i++)
    {
      if (self->tracks[i]->visible)
        {
          g_warn_if_fail (self->tracks[i] != track);
          return self->tracks[i];
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
  int         rm_pl,
  int         free,
  int         publish_events,
  int         recalc_graph)
{
  g_message ("removing %s",
             track->name);
  int idx =
    array_index_of (
      self->tracks,
      self->num_tracks,
      track);
  g_warn_if_fail (
    track->pos == idx);

  array_delete (
    self->tracks,
    self->num_tracks,
    track);
  tracklist_selections_remove_track (
    TRACKLIST_SELECTIONS, track);

  /* move all other tracks */
  for (int i = track->pos;
       i < self->num_tracks; i++)
    self->tracks[i]->pos = i;

  channel_disconnect (
    track->channel,
    rm_pl, F_NO_RECALC_GRAPH);

  if (free)
    {
      free_later (track, track_free);
    }

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  if (publish_events)
    EVENTS_PUSH (ET_TRACKS_REMOVED,
                 NULL);
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
  tracklist_remove_track (
    self,
    track,
    F_NO_REMOVE_PL,
    F_NO_FREE,
    F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);

  tracklist_insert_track (
    self,
    track,
    pos,
    F_NO_PUBLISH_EVENTS,
    recalc_graph);

  if (publish_events)
    EVENTS_PUSH (ET_TRACKS_MOVED, NULL);
}
