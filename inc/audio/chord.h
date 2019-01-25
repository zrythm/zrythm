/*
 * audio/chord.h - Chord
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
 * Chord related code.
 * See https://www.scales-chords.com
 * https://www.basicmusictheory.com/c-harmonic-minor-triad-chords
 */

#ifndef __AUDIO_CHORD_H__
#define __AUDIO_CHORD_H__

#include <stdint.h>

#include "audio/position.h"

typedef enum MusicalNote
{
  NOTE_C,
  NOTE_CS,
  NOTE_D,
  NOTE_DS,
  NOTE_E,
  NOTE_F,
  NOTE_FS,
  NOTE_G,
  NOTE_GS,
  NOTE_A,
  NOTE_AS,
  NOTE_B
} MusicalNote;

/**
 * Chord type.
 */
typedef enum ChordType
{
  CHORD_TYPE_MAJ,
  CHORD_TYPE_MIN,
  CHORD_TYPE_7,
  CHORD_TYPE_MIN_7,
  CHORD_TYPE_MAJ_7,
  CHORD_TYPE_6,
  CHORD_TYPE_MIN_6,
  CHORD_TYPE_6_9,
  CHORD_TYPE_5,
  CHORD_TYPE_9,
  CHORD_TYPE_MIN_9,
  CHORD_TYPE_MAJ_9,
  CHORD_TYPE_11,
  CHORD_TYPE_MIN_11,
  CHORD_TYPE_13,
  CHORD_TYPE_ADD_2,
  CHORD_TYPE_ADD_9,
  CHORD_TYPE_7_MINUS_5,
  CHORD_TYPE_7_PLUS_5,
  CHORD_TYPE_SUS2,
  CHORD_TYPE_SUS4,
  CHORD_TYPE_DIM,
  CHORD_TYPE_AUG
} ChordType;

typedef struct _ChordWidget ChordWidget;

/**
 * Chords are to be generated on demand.
 */
typedef struct Chord
{
  Position              pos; ///< chord object position (if used in chord track)
  uint8_t               has_bass; ///< has base note or not
  MusicalNote           root_note; ///< root note
  MusicalNote           bass_note; ///< bass note 1 octave below
  ChordType             type;
  unsigned char   notes[36]; ///< 3 octaves, 1st octave is for base note
                                  ///< starts at C always
  int                   inversion; ///< == 0 no inversion,
                                   ///< < 0 means highest note(s) drop an octave
                                   ///< > 0 means lowest note(s) receive an octave
  ChordWidget *         widget;
} Chord;

/**
 * Creates a chord.
 */
Chord *
chord_new (MusicalNote            root,
           uint8_t                has_bass,
           MusicalNote            bass,
           ChordType              type,
           int                    inversion);

/**
 * Returns the chord in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
chord_as_string (Chord * chord);

#endif
