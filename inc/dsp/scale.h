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

static const cyaml_strval_t musical_scale_type_strings[] = {
  { N_ ("Chromatic"),             SCALE_CHROMATIC             },
  { N_ ("Major"),                 SCALE_MAJOR                 },
  { N_ ("Minor"),                 SCALE_MINOR                 },
  { N_ ("Ionian"),                SCALE_IONIAN                },
  { N_ ("Dorian"),                SCALE_DORIAN                },
  { N_ ("Phrygian"),              SCALE_PHRYGIAN              },
  { N_ ("Lydian"),                SCALE_LYDIAN                },
  { N_ ("Mixolydian"),            SCALE_MIXOLYDIAN            },
  { N_ ("Aeolian"),               SCALE_AEOLIAN               },
  { N_ ("Locrian"),               SCALE_LOCRIAN               },
  { N_ ("Melodic Minor"),         SCALE_MELODIC_MINOR         },
  { N_ ("Harmonic Minor"),        SCALE_HARMONIC_MINOR        },
  { N_ ("Whole Tone"),            SCALE_WHOLE_TONE            },
  { N_ ("Major Pentatonic"),      SCALE_MAJOR_PENTATONIC      },
  { N_ ("Minor Pentatonic"),      SCALE_MINOR_PENTATONIC      },
  { N_ ("Octatonic Half Whole"),  SCALE_OCTATONIC_HALF_WHOLE  },
  { N_ ("Octatonic Whole Half"),  SCALE_OCTATONIC_WHOLE_HALF  },
  { N_ ("Acoustic"),              SCALE_ACOUSTIC              },
  { N_ ("Harmonic Major"),        SCALE_HARMONIC_MAJOR        },
  { N_ ("Phrygian Dominant"),     SCALE_PHRYGIAN_DOMINANT     },
  { N_ ("Major Locrian"),         SCALE_MAJOR_LOCRIAN         },
  { N_ ("Algerian"),              SCALE_ALGERIAN              },
  { N_ ("Augmented"),             SCALE_AUGMENTED             },
  { N_ ("Double Harmonic"),       SCALE_DOUBLE_HARMONIC       },
  { N_ ("Chinese"),               SCALE_CHINESE               },
  { N_ ("Diminished"),            SCALE_DIMINISHED            },
  { N_ ("Dominant Diminished"),   SCALE_DOMINANT_DIMINISHED   },
  { N_ ("Egyptian"),              SCALE_EGYPTIAN              },
  { N_ ("Eight Tone Spanish"),    SCALE_EIGHT_TONE_SPANISH    },
  { N_ ("Enigmatic"),             SCALE_ENIGMATIC             },
  { N_ ("Geez"),                  SCALE_GEEZ                  },
  { N_ ("Hindu"),                 SCALE_HINDU                 },
  { N_ ("Hirajoshi"),             SCALE_HIRAJOSHI             },
  { N_ ("Hungarian Gypsy"),       SCALE_HUNGARIAN_GYPSY       },
  { N_ ("Insen"),                 SCALE_INSEN                 },
  { N_ ("Neapolitan Major"),      SCALE_NEAPOLITAN_MAJOR      },
  { N_ ("Neapolitan Minor"),      SCALE_NEAPOLITAN_MINOR      },
  { N_ ("Oriental"),              SCALE_ORIENTAL              },
  { N_ ("Romanian Minor"),        SCALE_ROMANIAN_MINOR        },
  { N_ ("Altered"),               SCALE_ALTERED               },
  { N_ ("Maqam"),                 SCALE_MAQAM                 },
  { N_ ("Yo"),                    SCALE_YO                    },
  { N_ ("Bebop Locrian"),         SCALE_BEBOP_LOCRIAN         },
  { N_ ("Bebop Dominant"),        SCALE_BEBOP_DOMINANT        },
  { N_ ("Bebop Major"),           SCALE_BEBOP_MAJOR           },
  { N_ ("Super Locrian"),         SCALE_SUPER_LOCRIAN         },
  { N_ ("Enigmatic Minor"),       SCALE_ENIGMATIC_MINOR       },
  { N_ ("Composite"),             SCALE_COMPOSITE             },
  { N_ ("Bhairav"),               SCALE_BHAIRAV               },
  { N_ ("Hungarian Minor"),       SCALE_HUNGARIAN_MINOR       },
  { N_ ("Persian"),               SCALE_PERSIAN               },
  { N_ ("Iwato"),                 SCALE_IWATO                 },
  { N_ ("Kumoi"),                 SCALE_KUMOI                 },
  { N_ ("Pelog"),                 SCALE_PELOG                 },
  { N_ ("Prometheus"),            SCALE_PROMETHEUS            },
  { N_ ("Prometheus Neapolitan"), SCALE_PROMETHEUS_NEAPOLITAN },
  { N_ ("Prometheus Liszt"),      SCALE_PROMETHEUS_LISZT      },
  { N_ ("Balinese"),              SCALE_BALINESE              },
  { N_ ("RagaTodi"),              SCALE_RAGATODI              },
  { N_ ("Japanese 1"),            SCALE_JAPANESE1             },
  { N_ ("Japanese 2"),            SCALE_JAPANESE2             },
};

/**
 * Musical scale descriptor.
 */
typedef struct MusicalScale
{
  int schema_version;

  /** Identification of the scale (e.g. AEOLIAN). */
  MusicalScaleType type;

  /** Root key of the scale. */
  MusicalNote root_key;
} MusicalScale;

static const cyaml_schema_field_t musical_scale_fields_schema[] = {
  YAML_FIELD_INT (MusicalScale, schema_version),
  YAML_FIELD_ENUM (MusicalScale, type, musical_scale_type_strings),
  YAML_FIELD_ENUM (MusicalScale, root_key, musical_note_strings),

  CYAML_FIELD_END,
};

static const cyaml_schema_value_t musical_scale_schema = {
  YAML_VALUE_PTR (MusicalScale, musical_scale_fields_schema),
};

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
