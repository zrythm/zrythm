/*
 * audio/scale.h - Scale
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
 * Musical scales.
 *
 * See https://pianoscales.org/
 */

#ifndef __AUDIO_SCALE_H__
#define __AUDIO_SCALE_H__

typedef enum MusicalScaleType
{
  SCALE_ACOUSTIC,
  SCALE_AEOLIAN, ///< natural minor
  SCALE_ALGERIAN,
  SCALE_ALTERED,
  SCALE_AUGMENTED,
  SCALE_BEBOP_DOMINANT,
  SCALE_BLUES,
  SCALE_CHINESE,
  SCALE_CHROMATIC, ///< all keys
  SCALE_DIMINISHED,
  SCALE_DOMINANT_DIMINISHED,
  SCALE_DORIAN,
  SCALE_DOUBLE_HARMONIC,
  SCALE_EIGHT_TONE_SPANISH,
  SCALE_ENIGMATIC,
  SCALE_EGYPTIAN,
  SCALE_FLAMENCO,
  SCALE_GEEZ,
  SCALE_GYPSY,
  SCALE_HALF_DIMINISHED,
  SCALE_HARMONIC_MAJOR,
  SCALE_HARMONIC_MINOR,
  SCALE_HINDU,
  SCALE_HIRAJOSHI,
  SCALE_HUNGARIAN_GYPSY,
  SCALE_HUNGARIAN_MINOR,
  SCALE_IN,
  SCALE_INSEN,
  SCALE_IONIAN, ///< major
  SCALE_ISTRIAN,
  SCALE_IWATO,
  SCALE_LOCRIAN,
  SCALE_LYDIAN_AUGMENTED,
  SCALE_LYDIAN,
  SCALE_MAJOR_LOCRIAN,
  SCALE_MAJOR_PENTATONIC,
  SCALE_MAQAM,
  SCALE_MELODIC_MINOR,
  SCALE_MINOR_PENTATONIC,
  SCALE_MIXOLYDIAN,
  SCALE_NEAPOLITAN_MAJOR,
  SCALE_NEAPOLITAN_MINOR,
  SCALE_OCTATONIC_HALF_WHOLE,
  SCALE_OCTATONIC_WHOLE_HALF,
  SCALE_ORIENTAL,
  SCALE_PERSIAN,
  SCALE_PHRYGIAN_DOMINANT,
  SCALE_PHRYGIAN,
  SCALE_PROMETHEUS,
  SCALE_ROMANIAN_MINOR,
  SCALE_TRITONE,
  SCALE_UKRANIAN_DORIAN,
  SCALE_WHOLE_TONE,
  SCALE_YO
} MusicalScaleType;

/**
 * Major.
 */
const unsigned char scale_ionian[12] =
{
  1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1
};

const unsigned char scale_lydian[12] =
{
  1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1
};

const unsigned char scale_melodic_minor_ascending[12] =
{
  1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1
};

const unsigned char scale_melodic_minor_descending[12] =
{
  1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0
};

/**
 * Natural minor.
 */
const unsigned char scale_aeolian[12] =
{
  1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0
};

const unsigned char scale_harmonic_minor[12] =
{
  1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1
};

const unsigned char scale_chromatic[12] =
{
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const unsigned char scale_dorian[12] =
{
  1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0
};

const unsigned char scale_harmonic_major[12] =
{
  1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1
};

const unsigned char scale_major_pentatonic[12] =
{
  1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0
};

const unsigned char scale_minor_pentatonic[12] =
{
  1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0
};

const unsigned char scale_algerian[12] =
{
  1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1
};

const unsigned char scale_major_locrian[12] =
{
  1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0
};

const unsigned char scale_augmented[12] =
{
  1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1
};

const unsigned char scale_double_harmonic[12] =
{
  1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1
};

const unsigned char scale_chinese[12] =
{
  1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1
};

const unsigned char scale_diminished[12] =
{
  1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1
};

const unsigned char scale_dominant_diminished[12] =
{
  1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0
};

const unsigned char scale_egyptian[12] =
{
  1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0
};

const unsigned char scale_eight_tone_spanish[12] =
{
  1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0
};

const unsigned char scale_enigmatic[12] =
{
  1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1
};

const unsigned char scale_geez[12] =
{
  1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0
};

const unsigned char scale_hindu[12] =
{
  1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0
};

const unsigned char scale_hirajoshi[12] =
{
  1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0
};

const unsigned char scale_hungarian_gypsy[12] =
{
  1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1
};

const unsigned char scale_insen[12] =
{
  1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0
};

const unsigned char scale_neapolitan_minor[12] =
{
  1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1
};

const unsigned char scale_neapolitan_major[12] =
{
  1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
};

const unsigned char scale_octatonic_half_whole[12] =
{
  1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0
};

const unsigned char scale_octatonic_whole_half[12] =
{
  1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1
};

const unsigned char scale_oriental[12] =
{
  1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0
};

const unsigned char scale_whole_tone[12] =
{
  1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0
};

const unsigned char scale_romanian_minor[12] =
{
  1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0
};

const unsigned char scale_phrygian_dominant[12] =
{
  1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0
};

const unsigned char scale_altered[12] =
{
  1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0
};

const unsigned char scale_maqam[12] =
{
  1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1
};

const unsigned char scale_yo[12] =
{
  1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0
};

typedef enum MusicalNote MusicalNote;
typedef struct Chord Chord;

/**
 * To be generated at the beginning, and then copied and reused.
 */
typedef struct MusicalScale
{
  MusicalScaleType           type; ///< enum identification of scale
  MusicalNote                root_key;
  int                        has_asc_desc; ///< flag if scale has different notes
                                          ///< when ascending and descending
  const unsigned char        notes[12]; ///< notes in scale, not used if flag above
                                        ///< is 1
  const unsigned char        notes_asc[12]; ///< notes when ascending
  const unsigned char        notes_desc[12]; ///< notes when descending
  Chord *                    default_chords[12]; ///< default chords, as many as
                                                  ///< the notes in the scale
                                                  ///< triads with base note
  int                        num_notes;
} MusicalScale;

/**
 * Returns the scale in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
musical_scale_as_string (MusicalScale * scale);

#endif
