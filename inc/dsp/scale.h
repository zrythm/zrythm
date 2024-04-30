// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include "dsp/chord_descriptor.h"

#include <glib/gi18n.h>

/**
 * @addtogroup dsp
 *
 * @{
 */

#define SCALE_SCHEMA_VERSION 2

/**
 * Scale type (name) eg Aeolian.
 */
typedef enum MusicalScaleType
{
  /** All keys. */
  SCALE_CHROMATIC,

  /* --- popular scales --- */

  SCALE_MAJOR,

  /** Natural minor. */
  SCALE_MINOR,

  /** Major (same as SCALE_MAJOR). */
  SCALE_IONIAN,

  SCALE_DORIAN,
  SCALE_PHRYGIAN,
  SCALE_LYDIAN,
  SCALE_MIXOLYDIAN,

  /** Natural minor (same as SCALE_MINOR). */
  SCALE_AEOLIAN,

  SCALE_LOCRIAN,
  SCALE_MELODIC_MINOR,
  SCALE_HARMONIC_MINOR,
  SCALE_WHOLE_TONE,
  SCALE_MAJOR_PENTATONIC,
  SCALE_MINOR_PENTATONIC,
  SCALE_OCTATONIC_HALF_WHOLE,
  SCALE_OCTATONIC_WHOLE_HALF,

  /* --- exotic scales --- */

  /** Lydian dominant. */
  SCALE_ACOUSTIC,

  SCALE_HARMONIC_MAJOR,
  SCALE_PHRYGIAN_DOMINANT,
  SCALE_MAJOR_LOCRIAN,
  SCALE_ALGERIAN,
  SCALE_AUGMENTED,
  SCALE_DOUBLE_HARMONIC,
  SCALE_CHINESE,
  SCALE_DIMINISHED,
  SCALE_DOMINANT_DIMINISHED,
  SCALE_EGYPTIAN,
  SCALE_EIGHT_TONE_SPANISH,
  SCALE_ENIGMATIC,
  SCALE_GEEZ,
  SCALE_HINDU,
  SCALE_HIRAJOSHI,
  SCALE_HUNGARIAN_GYPSY,
  SCALE_INSEN,
  SCALE_NEAPOLITAN_MAJOR,
  SCALE_NEAPOLITAN_MINOR,
  SCALE_ORIENTAL,
  SCALE_ROMANIAN_MINOR,
  SCALE_ALTERED,
  SCALE_MAQAM,
  SCALE_YO,
  SCALE_BEBOP_LOCRIAN,
  SCALE_BEBOP_DOMINANT,
  SCALE_BEBOP_MAJOR,
  SCALE_SUPER_LOCRIAN,
  SCALE_ENIGMATIC_MINOR,
  SCALE_COMPOSITE,
  SCALE_BHAIRAV,
  SCALE_HUNGARIAN_MINOR,
  SCALE_PERSIAN,
  SCALE_IWATO,
  SCALE_KUMOI,
  SCALE_PELOG,
  SCALE_PROMETHEUS,
  SCALE_PROMETHEUS_NEAPOLITAN,
  SCALE_PROMETHEUS_LISZT,
  SCALE_BALINESE,
  SCALE_RAGATODI,
  SCALE_JAPANESE1,
  SCALE_JAPANESE2,

  /* --- TODO unimplemented --- */

  SCALE_BLUES,
  SCALE_FLAMENCO,
  SCALE_GYPSY,
  SCALE_HALF_DIMINISHED,
  SCALE_IN,
  SCALE_ISTRIAN,
  SCALE_LYDIAN_AUGMENTED,
  SCALE_TRITONE,
  SCALE_UKRANIAN_DORIAN,

  NUM_SCALES,
} MusicalScaleType;

/**
 * Musical scale descriptor.
 */
typedef struct MusicalScale
{
  /** Identification of the scale (e.g. AEOLIAN). */
  MusicalScaleType type;

  /** Root key of the scale. */
  MusicalNote root_key;
} MusicalScale;

/**
 * Creates new scale using type and root note.
 */
MusicalScale *
musical_scale_new (MusicalScaleType type, MusicalNote root);

/**
 * Returns the notes in the given scale.
 *
 * @param ascending Whether to get the notes when
 *   ascending or descending (some scales have
 *   different notes when rising/falling).
 */
const bool *
musical_scale_get_notes (MusicalScaleType scale_type, bool ascending);

/**
 * Returns the triads in the given scale.
 *
 * There will be as many chords are enabled notes
 * in the scale, and the rest of the array will be
 * filled with CHORD_TYPE_NONE.
 *
 * @param ascending Whether to get the triads when
 *   ascending or descending (some scales have
 *   different triads when rising/falling).
 */
RETURNS_NONNULL
const ChordType *
musical_scale_get_triad_types (MusicalScaleType scale_type, bool ascending);

/**
 * Clones the scale.
 */
MusicalScale *
musical_scale_clone (MusicalScale * src);

const char *
musical_scale_type_to_string (const MusicalScaleType type);

/**
 * Prints the MusicalScale to a string.
 *
 * MUST be free'd.
 */
char *
musical_scale_to_string (const MusicalScale * const self);

/**
 * Same as above but uses a buffer instead of
 * allocating.
 */
void
musical_scale_strcpy (MusicalScale * scale, char * buf);

/**
 * Returns 1 if the scales are equal.
 */
static inline int
musical_scale_is_equal (MusicalScale * a, MusicalScale * b)
{
  return a->type == b->type && a->root_key == b->root_key;
}

/**
 * Returns if all of the chord's notes are in
 * the scale.
 */
bool
musical_scale_contains_chord (
  const MusicalScale * const    scale,
  const ChordDescriptor * const chord);

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
 * @param note A note inside a single octave (0-11).
 */
bool
musical_scale_contains_note (const MusicalScale * scale, MusicalNote note);

/**
 * Returns the scale in human readable string.
 *
 * MUST be free'd by caller.
 */
char *
musical_scale_as_string (MusicalScale * scale);

/**
 * Frees the MusicalScale.
 */
void
musical_scale_free (MusicalScale * scale);

/**
 * @}
 */

#endif
