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
      TRACKLIST->tracks[ts->tracks[i]->pos];
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
      array_append (self->tracks,
                    self->num_tracks,
                    track);

      EVENTS_PUSH (ET_TRACK_CHANGED,
                   track);
      EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
                   NULL);
    }

  /* if recording is not already
   * on, turn these on */
  if (!track->recording && track->channel)
    {
      track_set_recording (track, 1);
      track->channel->record_set_automatically =
        1;
    }
}

/**
 * Selects all Track's.
 *
 * @param visible_only Only select visible tracks.
 */
void
tracklist_selections_select_all (
  TracklistSelections * ts,
  int                   visible_only)
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track->visible || !visible_only)
        {
          tracklist_selections_add_track (
            ts, track);
        }
    }

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

void
tracklist_selections_remove_track (
  TracklistSelections * ts,
  Track *             track)
{
  if (!array_contains (ts->tracks,
                       ts->num_tracks,
                       track))
    {
      EVENTS_PUSH (ET_TRACK_CHANGED,
                   track);
      EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
                   NULL);
      return;
    }

    /* if record mode was set automatically
     * when the track was selected, turn record
     * off */
  if (track->channel &&
      track->channel->record_set_automatically)
    {
      track_set_recording (track, 0);
      track->channel->record_set_automatically = 0;
    }

  array_delete (
    ts->tracks,
    ts->num_tracks,
    track);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

int
tracklist_selections_contains_track (
  TracklistSelections * self,
  Track *               track)
{
  return
    array_contains (
      self->tracks, self->num_tracks, track);
}

/**
 * For debugging.
 */
void
tracklist_selections_print (
  TracklistSelections * self)
{
  g_message ("------ tracklist selections ------");

  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      g_message ("[idx %d] %s (pos %d)",
                 i, track->name,
                 track->pos);
    }
  g_message ("-------- end --------");
}

/**
 * Selects a single track after clearing the
 * selections.
 */
void
tracklist_selections_select_single (
  TracklistSelections * ts,
  Track *               track)
{
  Track * _track;
  for (int i = ts->num_tracks - 1; i >= 0; i--)
    {
      _track = ts->tracks[i];
      tracklist_selections_remove_track (
        ts, _track);

      /* FIXME what if the track is deleted */
      EVENTS_PUSH (ET_TRACK_CHANGED,
                   _track);
    }

  tracklist_selections_add_track (
    ts, track);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

/**
 * Selects the last visible track after clearing the
 * selections.
 */
void
tracklist_selections_select_last_visible (
  TracklistSelections * ts)
{
  Track * track =
    tracklist_get_last_visible_track (TRACKLIST);;
  g_warn_if_fail (track);
  tracklist_selections_select_single (
    ts, track);
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
         (size_t) self->num_tracks,
         sizeof (Track *),
         sort_tracks_func);
}

/**
 * Toggle visibility of the selected tracks.
 */
void
tracklist_selections_toggle_visibility (
  TracklistSelections * ts)
{
  Track * track;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];
      track->visible = !track->visible;
    }

  EVENTS_PUSH (ET_TRACK_VISIBILITY_CHANGED, NULL);
}

/**
 * Clone the struct for copying, undoing, etc.
 */
TracklistSelections *
tracklist_selections_clone (
  TracklistSelections * self)
{
  TracklistSelections * new_ts =
    calloc (1, sizeof (TracklistSelections));

  TracklistSelections * src = TRACKLIST_SELECTIONS;

  for (int i = 0; i < src->num_tracks; i++)
    {
      Track * r = src->tracks[i];
      Track * new_r =
        track_clone (r);
      array_append (
        new_ts->tracks,
        new_ts->num_tracks,
        new_r);
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
