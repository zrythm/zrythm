/*
 * audio/tracklist.c - Tracklist backend
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
#include "gui/widgets/tracklist.h"
#include "utils/arrays.h"

Tracklist *
tracklist_new ()
{
  Tracklist * self = calloc (1, sizeof (Tracklist));

  /* add master */
  /*ChordTrack * chord_track = chord_track_default ();*/
  /*tracklist_add_track (self, chord_track);*/

  /* add chord track */
  ChordTrack * chord_track = chord_track_default ();
  tracklist_add_track (self,
                       (Track * )chord_track,
                       self->num_tracks);

  /* add each channel */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      tracklist_add_track (self,
                           channel->track,
                           self->num_tracks);
    }

  return self;
}

/**
 * Finds selected tracks and puts them in given array.
 */
void
tracklist_get_selected_tracks (Tracklist * tracklist,
                               Track **    selected_tracks,
                               int *       num_selected)
{
  *num_selected = 0;
  for (int i = 0; i < tracklist->num_tracks; i++)
    {
      Track * track = tracklist->tracks[i];
      if (track->selected)
        {
          selected_tracks[*num_selected++] = track;
        }
    }
}

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (Tracklist * tracklist,
                              Track **    visible_tracks,
                              int *       num_visible)
{
  *num_visible = 0;
  for (int i = 0; i < tracklist->num_tracks; i++)
    {
      Track * track = tracklist->tracks[i];
      if (track->visible)
        {
          visible_tracks[*num_visible++] = track;
        }
    }
}

int
tracklist_contains_master_track (Tracklist * tracklist)
{
  for (int i = 0; tracklist->num_tracks; i++)
    {
      Track * track = tracklist->tracks[i];
      if (track->type == TRACK_TYPE_MASTER)
        return 1;
    }
  return 0;
}

int
tracklist_contains_chord_track (Tracklist * tracklist)
{
  for (int i = 0; tracklist->num_tracks; i++)
    {
      Track * track = tracklist->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        return 1;
    }
  return 0;
}

/**
 * Adds given track to given spot in tracklist.
 */
void
tracklist_add_track (Tracklist * tracklist,
                     Track *     track,
                     int         pos)
{
  array_insert ((void **) tracklist->tracks,
                &tracklist->num_tracks,
                pos,
                (void *) track);
}

int
tracklist_get_track_pos (Tracklist * tracklist,
                         Track *     track)
{
  return array_index_of ((void **) tracklist->tracks,
                         tracklist->num_tracks,
                         (void *) track);
}
