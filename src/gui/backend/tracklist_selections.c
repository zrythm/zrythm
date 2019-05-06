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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/events.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

void
tracklist_selections_init_loaded (
  TracklistSelections * ts)
{
  int i;
  for (i = 0; i < ts->num_tracks; i++)
    ts->tracks[i] =
      project_get_track (ts->track_ids[i]);
}

/**
 * Returns if there are any selections.
 */
int
tracklist_selections_has_any (
  TracklistSelections * ts)
{
  if (ts->num_tracks > 0)
    return 1;
  else
    return 0;
}

/**
 * Gets highest track in the selections.
 */
Track *
tracklist_selections_get_highest_track (
  TracklistSelections * ts)
{
  Track * track;
  int min_pos = 1000;
  Track * min_track = NULL;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];

      if (track->pos < min_pos)
        {
          min_pos = track->pos;
          min_track = track;
        }
    }

  return min_track;
}

/**
 * Gets lowest track in the selections.
 */
Track *
tracklist_selections_get_lowest_track (
  TracklistSelections * ts)
{
  Track * track;
  int max_pos = -1;
  Track * max_track = NULL;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];

      if (track->pos > max_pos)
        {
          max_pos = track->pos;
          max_track = track;
        }
    }

  return max_track;
}

/**
 * Adds a Track to the selections.
 */
void
tracklist_selections_add_track (
  TracklistSelections * self,
  Track *               track)
{
  if (!array_contains (self->tracks,
                      self->num_tracks,
                      track))
    {
      self->track_ids[self->num_tracks] =
        track->id;
      array_append (self->tracks,
                    self->num_tracks,
                    track);

      EVENTS_PUSH (ET_TRACK_CHANGED,
                   track);
      EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
                   NULL);
    }
}

void
tracklist_selections_remove_track (
  TracklistSelections * ts,
  Track *             r)
{
  if (!array_contains (ts->tracks,
                       ts->num_tracks,
                       r))
    {
      EVENTS_PUSH (ET_TRACK_CHANGED,
                   r);
      EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
                   NULL);
      return;
    }

  array_double_delete (
    ts->tracks,
    ts->track_ids,
    ts->num_tracks,
    r,
    r->id);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

int
tracklist_selections_contains_track (
  TracklistSelections * self,
  Track *               track)
{
  return array_contains (self->tracks,
                      self->num_tracks,
                      track);
}

/**
 * For debugging.
 */
void
tracklist_selections_gprint (
  TracklistSelections * self)
{
  g_message ("------ tracklist selections ------");

  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      g_message ("[idx %d] %s (id %d) (pos %d)",
                 i, self->tracks[i]->name,
                 track->id,
                 self->tracks[i]->pos);
      g_message (">>>> track id %d <<<<<",
                 self->track_ids[i]);
    }
  g_message ("-------- end --------");
}

/**
 * Clears selections.
 */
void
tracklist_selections_clear (
  TracklistSelections * ts)
{
  Track * track;
  for (int i = ts->num_tracks - 1; i >= 0; i--)
    {
      track = ts->tracks[i];
      tracklist_selections_remove_track (
        ts, track);

      /* FIXME what if the track is deleted */
      EVENTS_PUSH (ET_TRACK_CHANGED,
                   track);
    }

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);

  g_message ("cleared tracklist selections");
}

static int
sort_tracks_func (const void *a, const void *b)
{
  Track * aa = * (Track * const *) a;
  Track * bb = * (Track * const *) b;
  g_message ("aa pos %d bb pos %d",
             aa->pos, bb->pos);
  return aa->pos > bb->pos;
}

/**
 * Sorts the tracks by position.
 */
void
tracklist_selections_sort (
  TracklistSelections * self)
{
  qsort (self->tracks,
         self->num_tracks,
         sizeof (Track *),
         sort_tracks_func);

  /* also sort the IDs */
  for (int i = 0; i < self->num_tracks; i++)
    self->track_ids[i] = self->tracks[i]->id;
}

/**
 * Clone the struct for copying, undoing, etc.
 */
TracklistSelections *
tracklist_selections_clone ()
{
  TracklistSelections * new_ts =
    calloc (1, sizeof (TracklistSelections));

  TracklistSelections * src = TRACKLIST_SELECTIONS;

  for (int i = 0; i < src->num_tracks; i++)
    {
      Track * r = src->tracks[i];
      Track * new_r =
        track_clone (r);
      array_double_append (
        new_ts->tracks,
        new_ts->track_ids,
        new_ts->num_tracks,
        new_r,
        new_r->id);
    }

  return new_ts;
}

void
tracklist_selections_paste_to_pos (
  TracklistSelections * ts,
  int           pos)
{
  /* TODO */
  g_warn_if_reached ();
  return;
}

void
tracklist_selections_free (TracklistSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  TracklistSelections, tracklist_selections)
DESERIALIZE_SRC (
  TracklistSelections, tracklist_selections)
PRINT_YAML_SRC (
  TracklistSelections, tracklist_selections)
