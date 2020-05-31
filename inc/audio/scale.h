/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdint.h>

#include "audio/chord_descriptor.h"

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Scale type (name) eg Aeolian.
 */
typedef enum MusicalScaleType
{
  SCALE_CHROMATIC, ///< all keys
  SCALE_IONIAN, ///< major
  SCALE_AEOLIAN, ///< natural minor
  SCALE_HARMONIC_MINOR,

  /* rest TODO */
  SCALE_ACOUSTIC,
  SCALE_ALGERIAN,
  SCALE_ALTERED,
  SCALE_AUGMENTED,
  SCALE_BEBOP_DOMINANT,
  SCALE_BLUES,
  SCALE_CHINESE,
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
  SCALE_HINDU,
  SCALE_HIRAJOSHI,
  SCALE_HUNGARIAN_GYPSY,
  SCALE_HUNGARIAN_MINOR,
  SCALE_IN,
  SCALE_INSEN,
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

//const unsigned char scale_lydian[12] =
//{
  //1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1
//};

//const unsigned char scale_chromatic[12] =
//{
  //1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
//};

//const unsigned char scale_dorian[12] =
//{
  //1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0
//};

//const unsigned char scale_harmonic_major[12] =
//{
  //1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1
//};

//const unsigned char scale_major_pentatonic[12] =
//{
  //1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0
//};

//const unsigned char scale_minor_pentatonic[12] =
//{
  //1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0
//};

//const unsigned char scale_algerian[12] =
//{
  //1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1
//};

//const unsigned char scale_major_locrian[12] =
//{
  //1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0
//};

//const unsigned char scale_augmented[12] =
//{
  //1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1
//};

//const unsigned char scale_double_harmonic[12] =
//{
  //1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1
//};

//const unsigned char scale_chinese[12] =
//{
  //1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1
//};

//const unsigned char scale_diminished[12] =
//{
  //1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1
//};

//const unsigned char scale_dominant_diminished[12] =
//{
  //1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0
//};

//const unsigned char scale_egyptian[12] =
//{
  //1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0
//};

//const unsigned char scale_eight_tone_spanish[12] =
//{
  //1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0
//};

//const unsigned char scale_enigmatic[12] =
//{
  //1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1
//};

//const unsigned char scale_geez[12] =
//{
  //1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0
//};

//const unsigned char scale_hindu[12] =
//{
  //1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0
//};

//const unsigned char scale_hirajoshi[12] =
//{
  //1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0
//};

//const unsigned char scale_hungarian_gypsy[12] =
//{
  //1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1
//};

//const unsigned char scale_insen[12] =
//{
  //1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0
//};

//const unsigned char scale_neapolitan_minor[12] =
//{
  //1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1
//};

//const unsigned char scale_neapolitan_major[12] =
//{
  //1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
//};

//const unsigned char scale_octatonic_half_whole[12] =
//{
  //1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0
//};

//const unsigned char scale_octatonic_whole_half[12] =
//{
  //1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1
//};

//const unsigned char scale_oriental[12] =
//{
  //1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0
//};

//const unsigned char scale_whole_tone[12] =
//{
  //1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0
//};

//const unsigned char scale_romanian_minor[12] =
//{
  //1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0
//};

//const unsigned char scale_phrygian_dominant[12] =
//{
  //1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0
//};

//const unsigned char scale_altered[12] =
//{
  //1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0
//};

//const unsigned char scale_maqam[12] =
//{
  //1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1
//};

//const unsigned char scale_yo[12] =
//{
  //1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0
//};

/**
 * To be generated at the beginning, and then copied and reused.
 */
typedef struct MusicalScale
{
  /** Identification of the scale (e.g. AEOLIAN). */
  MusicalScaleType   type;

  /** Root key of the scale. */
  MusicalNote        root_key;

  /** Flag if scale has different notes when
   * ascending and descending. */
  int                has_asc_desc;

  /** Notes in the scale (if has_asc_desc is 0). */
  int                notes[12];

  /** Notes when ascending (if has_asc_desc is 1). */
  int                notes_asc[12];

  /** Notes when descending (if has_asc_desc is
   * 0). */
  int                notes_desc[12];

  /**
   * Default triad chords with base note, as many
   * as the notes in the scale.
   *
   * Triads with base note.
   */
  ChordDescriptor *  default_chords[12];

  /** Note count (1s). */
  int                num_notes;
} MusicalScale;

static const cyaml_strval_t
  musical_scale_type_strings[] =
{
  { "Acoustic",     SCALE_ACOUSTIC    },
  { "Aeolian",      SCALE_AEOLIAN   },
  { "Algerian",     SCALE_ALGERIAN   },
  { "Altered",      SCALE_ALTERED   },
  { "Augmented",    SCALE_AUGMENTED   },
};

static const cyaml_schema_field_t
  musical_scale_fields_schema[] =
{
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    MusicalScale, type, musical_scale_type_strings,
    CYAML_ARRAY_LEN (musical_scale_type_strings)),
  CYAML_FIELD_ENUM (
    "root_key", CYAML_FLAG_DEFAULT,
    MusicalScale, root_key, musical_note_strings,
    CYAML_ARRAY_LEN (musical_note_strings)),
  CYAML_FIELD_SEQUENCE_FIXED (
    "notes", CYAML_FLAG_OPTIONAL,
    MusicalScale, notes, &int_schema, 12),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  musical_scale_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    MusicalScale, musical_scale_fields_schema),
};

/**
 * Creates new scale using type and root note.
 */
MusicalScale *
musical_scale_new (
  MusicalScaleType type,
  MusicalNote      root);

/**
 * Clones the scale.
 */
MusicalScale *
musical_scale_clone (
  MusicalScale * src);

/**
 * Prints the MusicalScale to a string.
 *
 * MUST be free'd.
 */
char *
musical_scale_to_string (
  MusicalScale * scale);

/**
 * Same as above but uses a buffer instead of
 * allocating.
 */
void
musical_scale_strcpy (
  MusicalScale * scale,
  char *         buf);

/**
 * Returns 1 if the scales are equal.
 */
static inline int
musical_scale_is_equal (
  MusicalScale * a,
  MusicalScale * b)
{
  /* TODO */
  return a->type == b->type;
}

/**
 * Returns if all of the chord's notes are in
 * the scale.
 */
int
musical_scale_is_chord_in_scale (
  MusicalScale * scale,
  ChordDescriptor * chord);

/**
 * Returns if the accent is in the scale.
 */
int
musical_scale_is_accent_in_scale (
  MusicalScale * scale,
  MusicalNote    chord_root,
  ChordType      type,
  ChordAccent    chord_accent);

/**
 * Returns if the given key is in the given
 * MusicalScale.
 *
 * @param key A note inside a single octave (0-11).
 */
int
musical_scale_is_key_in_scale (
  MusicalScale * scale,
  MusicalNote    key);

/**
 * Returns the scale in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
musical_scale_as_string (
  MusicalScale * scale);

/**
 * Frees the MusicalScale.
 */
void
musical_scale_free (
  MusicalScale * scale);

/**
 * @}
 */

#endif
