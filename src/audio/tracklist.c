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

/**
 * Initializes the tracklist.
 *
 * Note: mixer and master channel/track, chord track and
 * each channel must be initialized at this point.
 */
void
tracklist_init (Tracklist * self)
{
  /* add master track */
  g_assert (MIXER);
  g_assert (MIXER->master);
  g_assert (MIXER->master->track);
  tracklist_append_track ((Track *) MIXER->master->track);

  /* add chord track */
  g_assert (CHORD_TRACK);
  tracklist_append_track ((Track *) CHORD_TRACK);

  /* add each channel */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      g_assert (channel);
      g_assert (channel->track);
      tracklist_append_track (channel->track);
    }
}

/**
 * Finds selected tracks and puts them in given array.
 */
void
tracklist_get_selected_tracks (Track **    selected_tracks,
                               int *       num_selected)
{
  *num_selected = 0;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->selected)
        {
          selected_tracks[(*num_selected)++] = track;
        }
    }
}

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (Track **    visible_tracks,
                              int *       num_visible)
{
  *num_visible = 0;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->visible)
        {
          visible_tracks[*num_visible++] = track;
        }
    }
}

int
tracklist_contains_master_track ()
{
  for (int i = 0; TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_MASTER)
        return 1;
    }
  return 0;
}

int
tracklist_contains_chord_track ()
{
  for (int i = 0; TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        return 1;
    }
  return 0;
}

/**
 * Adds given track to given spot in tracklist.
 */
void
tracklist_add_track (Track *     track,
                     int         pos)
{
  TRACKLIST->track_ids[pos] =
    track->id;
  array_insert (TRACKLIST->tracks,
                TRACKLIST->num_tracks,
                pos,
                track);
      EVENTS_PUSH (ET_TRACK_ADDED, track);
}

ChordTrack *
tracklist_get_chord_track ()
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        {
          return (ChordTrack *) track;
        }
    }
  g_assert_not_reached ();
}

void
tracklist_append_track (Track *     track)
{
  TRACKLIST->track_ids[TRACKLIST->num_tracks] =
    track->id;
  array_append (TRACKLIST->tracks,
                TRACKLIST->num_tracks,
                track);
}

int
tracklist_get_track_pos (Track *     track)
{
  return array_index_of ((void **) TRACKLIST->tracks,
                         TRACKLIST->num_tracks,
                         (void *) track);
}

int
tracklist_get_last_visible_pos ()
{
  for (int i = TRACKLIST->num_tracks - 1; i >= 0; i--)
    {
      if (TRACKLIST->tracks[i]->visible)
        {
          return i;
        }
    }
  g_assert_not_reached ();
  return -1;
}

Track*
tracklist_get_last_visible_track ()
{
  for (int i = TRACKLIST->num_tracks - 1; i >= 0; i--)
    {
      if (TRACKLIST->tracks[i]->visible)
        {
          return TRACKLIST->tracks[i];
        }
    }
  g_assert_not_reached ();
  return NULL;
}

Track *
tracklist_get_first_visible_track ()
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      if (TRACKLIST->tracks[i]->visible)
        {
          return TRACKLIST->tracks[i];
        }
    }
  g_assert_not_reached ();
  return NULL;
}

Track *
tracklist_get_prev_visible_track (Track * track)
{
  for (int i = tracklist_get_track_pos (track);
       i >= 0; i--)
    {
      if (TRACKLIST->tracks[i]->visible)
        {
          return TRACKLIST->tracks[i];
        }
    }
  g_assert_not_reached ();
  return NULL;
}

Track *
tracklist_get_next_visible_track (Track * track)
{
  for (int i = tracklist_get_track_pos (track);
       i < TRACKLIST->num_tracks; i++)
    {
      if (TRACKLIST->tracks[i]->visible)
        {
          return TRACKLIST->tracks[i];
        }
    }
  g_assert_not_reached ();
  return NULL;
}

void
tracklist_remove_track (Track *     track)
{
  array_delete (TRACKLIST->tracks,
                TRACKLIST->num_tracks,
                track);
}
