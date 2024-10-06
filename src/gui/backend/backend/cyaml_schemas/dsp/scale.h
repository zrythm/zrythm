// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Musical scale schema.
 */

#ifndef __SCHEMAS_AUDIO_SCALE_H__
#define __SCHEMAS_AUDIO_SCALE_H__

#include <cstdint>

#include "gui/backend/backend/cyaml_schemas/dsp/chord_descriptor.h"

typedef enum MusicalScaleType_v2
{
  SCALE_CHROMATIC_v2,
  SCALE_MAJOR_v2,
  SCALE_MINOR_v2,
  SCALE_IONIAN_v2,
  SCALE_DORIAN_v2,
  SCALE_PHRYGIAN_v2,
  SCALE_LYDIAN_v2,
  SCALE_MIXOLYDIAN_v2,
  SCALE_AEOLIAN_v2,
  SCALE_LOCRIAN_v2,
  SCALE_MELODIC_MINOR_v2,
  SCALE_HARMONIC_MINOR_v2,
  SCALE_WHOLE_TONE_v2,
  SCALE_MAJOR_PENTATONIC_v2,
  SCALE_MINOR_PENTATONIC_v2,
  SCALE_OCTATONIC_HALF_WHOLE_v2,
  SCALE_OCTATONIC_WHOLE_HALF_v2,
  SCALE_ACOUSTIC_v2,
  SCALE_HARMONIC_MAJOR_v2,
  SCALE_PHRYGIAN_DOMINANT_v2,
  SCALE_MAJOR_LOCRIAN_v2,
  SCALE_ALGERIAN_v2,
  SCALE_AUGMENTED_v2,
  SCALE_DOUBLE_HARMONIC_v2,
  SCALE_CHINESE_v2,
  SCALE_DIMINISHED_v2,
  SCALE_DOMINANT_DIMINISHED_v2,
  SCALE_EGYPTIAN_v2,
  SCALE_EIGHT_TONE_SPANISH_v2,
  SCALE_ENIGMATIC_v2,
  SCALE_GEEZ_v2,
  SCALE_HINDU_v2,
  SCALE_HIRAJOSHI_v2,
  SCALE_HUNGARIAN_GYPSY_v2,
  SCALE_INSEN_v2,
  SCALE_NEAPOLITAN_MAJOR_v2,
  SCALE_NEAPOLITAN_MINOR_v2,
  SCALE_ORIENTAL_v2,
  SCALE_ROMANIAN_MINOR_v2,
  SCALE_ALTERED_v2,
  SCALE_MAQAM_v2,
  SCALE_YO_v2,
  SCALE_BEBOP_LOCRIAN_v2,
  SCALE_BEBOP_DOMINANT_v2,
  SCALE_BEBOP_MAJOR_v2,
  SCALE_SUPER_LOCRIAN_v2,
  SCALE_ENIGMATIC_MINOR_v2,
  SCALE_COMPOSITE_v2,
  SCALE_BHAIRAV_v2,
  SCALE_HUNGARIAN_MINOR_v2,
  SCALE_PERSIAN_v2,
  SCALE_IWATO_v2,
  SCALE_KUMOI_v2,
  SCALE_PELOG_v2,
  SCALE_PROMETHEUS_v2,
  SCALE_PROMETHEUS_NEAPOLITAN_v2,
  SCALE_PROMETHEUS_LISZT_v2,
  SCALE_BALINESE_v2,
  SCALE_RAGATODI_v2,
  SCALE_JAPANESE1_v2,
  SCALE_JAPANESE2_v2,
  SCALE_BLUES_v2,
  SCALE_FLAMENCO_v2,
  SCALE_GYPSY_v2,
  SCALE_HALF_DIMINISHED_v2,
  SCALE_IN_v2,
  SCALE_ISTRIAN_v2,
  SCALE_LYDIAN_AUGMENTED_v2,
  SCALE_TRITONE_v2,
  SCALE_UKRANIAN_DORIAN_v2,
  NUM_SCALES_v2,
} MusicalScaleType_v2;

static const cyaml_strval_t musical_scale_type_strings_v2[] = {
  { ("Chromatic"),             SCALE_CHROMATIC_v2             },
  { ("Major"),                 SCALE_MAJOR_v2                 },
  { ("Minor"),                 SCALE_MINOR_v2                 },
  { ("Ionian"),                SCALE_IONIAN_v2                },
  { ("Dorian"),                SCALE_DORIAN_v2                },
  { ("Phrygian"),              SCALE_PHRYGIAN_v2              },
  { ("Lydian"),                SCALE_LYDIAN_v2                },
  { ("Mixolydian"),            SCALE_MIXOLYDIAN_v2            },
  { ("Aeolian"),               SCALE_AEOLIAN_v2               },
  { ("Locrian"),               SCALE_LOCRIAN_v2               },
  { ("Melodic Minor"),         SCALE_MELODIC_MINOR_v2         },
  { ("Harmonic Minor"),        SCALE_HARMONIC_MINOR_v2        },
  { ("Whole Tone"),            SCALE_WHOLE_TONE_v2            },
  { ("Major Pentatonic"),      SCALE_MAJOR_PENTATONIC_v2      },
  { ("Minor Pentatonic"),      SCALE_MINOR_PENTATONIC_v2      },
  { ("Octatonic Half Whole"),  SCALE_OCTATONIC_HALF_WHOLE_v2  },
  { ("Octatonic Whole Half"),  SCALE_OCTATONIC_WHOLE_HALF_v2  },
  { ("Acoustic"),              SCALE_ACOUSTIC_v2              },
  { ("Harmonic Major"),        SCALE_HARMONIC_MAJOR_v2        },
  { ("Phrygian Dominant"),     SCALE_PHRYGIAN_DOMINANT_v2     },
  { ("Major Locrian"),         SCALE_MAJOR_LOCRIAN_v2         },
  { ("Algerian"),              SCALE_ALGERIAN_v2              },
  { ("Augmented"),             SCALE_AUGMENTED_v2             },
  { ("Double Harmonic"),       SCALE_DOUBLE_HARMONIC_v2       },
  { ("Chinese"),               SCALE_CHINESE_v2               },
  { ("Diminished"),            SCALE_DIMINISHED_v2            },
  { ("Dominant Diminished"),   SCALE_DOMINANT_DIMINISHED_v2   },
  { ("Egyptian"),              SCALE_EGYPTIAN_v2              },
  { ("Eight Tone Spanish"),    SCALE_EIGHT_TONE_SPANISH_v2    },
  { ("Enigmatic"),             SCALE_ENIGMATIC_v2             },
  { ("Geez"),                  SCALE_GEEZ_v2                  },
  { ("Hindu"),                 SCALE_HINDU_v2                 },
  { ("Hirajoshi"),             SCALE_HIRAJOSHI_v2             },
  { ("Hungarian Gypsy"),       SCALE_HUNGARIAN_GYPSY_v2       },
  { ("Insen"),                 SCALE_INSEN_v2                 },
  { ("Neapolitan Major"),      SCALE_NEAPOLITAN_MAJOR_v2      },
  { ("Neapolitan Minor"),      SCALE_NEAPOLITAN_MINOR_v2      },
  { ("Oriental"),              SCALE_ORIENTAL_v2              },
  { ("Romanian Minor"),        SCALE_ROMANIAN_MINOR_v2        },
  { ("Altered"),               SCALE_ALTERED_v2               },
  { ("Maqam"),                 SCALE_MAQAM_v2                 },
  { ("Yo"),                    SCALE_YO_v2                    },
  { ("Bebop Locrian"),         SCALE_BEBOP_LOCRIAN_v2         },
  { ("Bebop Dominant"),        SCALE_BEBOP_DOMINANT_v2        },
  { ("Bebop Major"),           SCALE_BEBOP_MAJOR_v2           },
  { ("Super Locrian"),         SCALE_SUPER_LOCRIAN_v2         },
  { ("Enigmatic Minor"),       SCALE_ENIGMATIC_MINOR_v2       },
  { ("Composite"),             SCALE_COMPOSITE_v2             },
  { ("Bhairav"),               SCALE_BHAIRAV_v2               },
  { ("Hungarian Minor"),       SCALE_HUNGARIAN_MINOR_v2       },
  { ("Persian"),               SCALE_PERSIAN_v2               },
  { ("Iwato"),                 SCALE_IWATO_v2                 },
  { ("Kumoi"),                 SCALE_KUMOI_v2                 },
  { ("Pelog"),                 SCALE_PELOG_v2                 },
  { ("Prometheus"),            SCALE_PROMETHEUS_v2            },
  { ("Prometheus Neapolitan"), SCALE_PROMETHEUS_NEAPOLITAN_v2 },
  { ("Prometheus Liszt"),      SCALE_PROMETHEUS_LISZT_v2      },
  { ("Balinese"),              SCALE_BALINESE_v2              },
  { ("RagaTodi"),              SCALE_RAGATODI_v2              },
  { ("Japanese 1"),            SCALE_JAPANESE1_v2             },
  { ("Japanese 2"),            SCALE_JAPANESE2_v2             },
};

typedef struct MusicalScale_v2
{
  int                 schema_version;
  MusicalScaleType_v2 type;
  MusicalNote_v1      root_key;
} MusicalScale_v2;

static const cyaml_schema_field_t musical_scale_fields_schema_v2[] = {
  YAML_FIELD_INT (MusicalScale_v2, schema_version),
  YAML_FIELD_ENUM (MusicalScale_v2, type, musical_scale_type_strings_v2),
  YAML_FIELD_ENUM (MusicalScale_v2, root_key, musical_note_strings_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t musical_scale_schema_v2 = {
  YAML_VALUE_PTR (MusicalScale_v2, musical_scale_fields_schema_v2),
};

#endif
