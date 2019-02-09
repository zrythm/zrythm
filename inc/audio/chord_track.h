/*
 * audio/chord_track.h - Chord track
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

#ifndef __AUDIO_CHORD_TRACK_H__
#define __AUDIO_CHORD_TRACK_H__

#include <stdint.h>

#include "audio/track.h"

#define CHORD_TRACK PROJECT->chord_track

typedef struct Chord Chord;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct MusicalScale MusicalScale;

typedef struct ChordTrack
{
  Track                   parent;
  MusicalScale *          scale;
  Chord *                 chords[600];
  int                     num_chords;
  char *                  name;
  ChordTrackWidget *      widget;
  GdkRGBA                 color;
} ChordTrack;

/**
 * Creates a new chord track using the given scale.
 */
ChordTrack *
chord_track_new (MusicalScale * scale);

/**
 * Creates chord track using default scale.
 */
ChordTrack *
chord_track_default ();

void
chord_track_add_chord (ChordTrack * self,
                       Chord *      chord);

void
chord_track_remove_chord (ChordTrack * self,
                          Chord *      chord);

void
chord_track_free (ChordTrack * self);

#endif
