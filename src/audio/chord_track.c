/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/chord_track.h"
#include "audio/scale.h"
#include "audio/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Creates a new chord track.
 */
ChordTrack *
chord_track_new ()
{
  ChordTrack * self = calloc (1, sizeof (ChordTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_CHORD;
  track_init (track);

  self->name = g_strdup (_("Chords"));

  gdk_rgba_parse (&self->color, "#0328fa");

  return self;
}

/**
 * Adds a chord to the chord track.
 */
void
chord_track_add_chord (
  ChordTrack * track,
  ChordObject *     chord,
  int          gen_widget)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_CHORD && chord);
  g_warn_if_fail (
    chord->obj_info.counterpart ==
      AOI_COUNTERPART_MAIN);
  g_warn_if_fail (
    chord->obj_info.main &&
    chord->obj_info.main_trans &&
    chord->obj_info.lane &&
    chord->obj_info.lane_trans);

  chord_object_set_track (chord, track);
  array_append (track->chords,
                track->num_chords,
                chord);

  if (gen_widget)
    chord_object_gen_widget (chord);

  EVENTS_PUSH (ET_CHORD_OBJECT_CREATED, chord);
}

/**
 * Adds a ChordObject to the Track.
 *
 * @param gen_widget Create a widget for the chord.
 */
void
chord_track_add_scale (
  ChordTrack *  track,
  ScaleObject * scale,
  int           gen_widget)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_CHORD && scale);
  g_warn_if_fail (
    scale->obj_info.counterpart ==
      AOI_COUNTERPART_MAIN);
  g_warn_if_fail (
    scale->obj_info.main &&
    scale->obj_info.main_trans &&
    scale->obj_info.lane &&
    scale->obj_info.lane_trans);

  scale_object_set_track (scale, track);
  array_append (track->scales,
                track->num_scales,
                scale);

  if (gen_widget)
    scale_object_gen_widget (scale);

  EVENTS_PUSH (ET_SCALE_OBJECT_CREATED, scale);
}

/**
 * Returns the ScaleObject at the given Position
 * in the TimelineArranger.
 */
ScaleObject *
chord_track_get_scale_at_pos (
  const Track * ct,
  const Position * pos)
{
  ScaleObject * scale = NULL;
  for (int i = ct->num_scales - 1; i >= 0; i--)
    {
      scale = ct->scales[i];
      if (position_is_before_or_equal (
            &scale->pos, pos))
        return scale;
    }
  return NULL;
}

/**
 * Returns the ChordObject at the given Position
 * in the TimelineArranger.
 */
ChordObject *
chord_track_get_chord_at_pos (
  const Track * ct,
  const Position * pos)
{
  ChordObject * chord = NULL;
  for (int i = ct->num_chords - 1; i >= 0; i--)
    {
      chord = ct->chords[i];
      if (position_is_before_or_equal (
            &chord->pos, pos))
        return chord;
    }
  return NULL;
}

/**
 * Removes a chord from the chord Track.
 */
void
chord_track_remove_chord (
  ChordTrack *  self,
  ChordObject * chord,
  int           free)
{
  array_delete (self->chords,
                self->num_chords,
                chord);

  if (free)
    free_later (chord, chord_object_free);
}

/**
 * Removes a scale from the chord Track.
 */
void
chord_track_remove_scale (
  ChordTrack *  self,
  ScaleObject * scale,
  int free)
{
  array_delete (self->scales,
                self->num_scales,
                scale);
  if (free)
    free_later (scale, scale_object_free);
}

void
chord_track_free (ChordTrack * self)
{

}
