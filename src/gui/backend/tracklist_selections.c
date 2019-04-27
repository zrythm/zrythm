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
  /* TODO */
  g_warn_if_reached ();
  return NULL;
}

/**
 * Gets lowest track in the selections.
 */
Track *
tracklist_selections_get_lowest_track (
  TracklistSelections * ts)
{
  /* TODO */
  g_warn_if_reached ();
  return NULL;
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
 * Clears selections.
 */
void
tracklist_selections_clear (
  TracklistSelections * ts)
{
  Track * track;
  for (int i = 0; i < ts->num_tracks; i++)
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
      array_append (new_ts->tracks,
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
