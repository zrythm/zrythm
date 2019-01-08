/*
 * audio/chord_track.c - Chord track
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

/**
 * \file
 *
 * Object to hold information for the chord track.
 *
 * Contains project scale, chord markers, etc.
 */

#include <stdlib.h>

#include "audio/chord_track.h"
#include "audio/scale.h"
#include "audio/track.h"
#include "utils/arrays.h"

/**
 * Creates a new chord track using the given scale.
 */
ChordTrack *
chord_track_new (MusicalScale * scale)
{
  ChordTrack * self = calloc (1, sizeof (ChordTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_CHORD;
  track_init (track);

  self->scale = scale;

  gdk_rgba_parse (&self->color, "#0328fa");

  return self;
}

void
chord_track_add_chord (ChordTrack * self,
                       Chord *      chord)
{
  array_append ((void **) self->chords,
                &self->num_chords,
                (void *) chord);
}

void
chord_track_remove_chord (ChordTrack * self,
                          Chord *      chord)
{
  array_delete ((void **) self->chords,
                &self->num_chords,
                (void *) chord);
}

/**
 * Creates chord track using default scale.
 */
ChordTrack *
chord_track_default ()
{
  MusicalScale * scale =
    musical_scale_new (SCALE_AEOLIAN, // natural minor
                       NOTE_A);

  return chord_track_new (scale);
}

void
chord_track_free (ChordTrack * self)
{

}
