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

typedef struct Chord Chord;
typedef struct ChordTrackWidget ChordTrackWidget;
typedef struct MusicalScale MusicalScale;

typedef struct ChordTrack
{
  MusicalScale *          scale;
  Chord *                 chords[600];
  uint8_t                 num_chords;
  ChordTrackWidget *      widget;
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

#endif
