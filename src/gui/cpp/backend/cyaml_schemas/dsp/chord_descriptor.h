// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Chord descriptor schema.
 */

#ifndef __SCHEMAS_AUDIO_CHORD_DESCRIPTOR_H__
#define __SCHEMAS_AUDIO_CHORD_DESCRIPTOR_H__

#include <cstdint>

#include "common/dsp/position.h"
#include "common/utils/yaml.h"

typedef enum MusicalNote_v1
{
  NOTE_C_v1,
  NOTE_CS_v1,
  NOTE_D_v1,
  NOTE_DS_v1,
  NOTE_E_v1,
  NOTE_F_v1,
  NOTE_FS_v1,
  NOTE_G_v1,
  NOTE_GS_v1,
  NOTE_A_v1,
  NOTE_AS_v1,
  NOTE_B_v1,
} MusicalNote_v1;

static const cyaml_strval_t musical_note_strings_v1[] = {
  { "C",  NOTE_C_v1  },
  { "C#", NOTE_CS_v1 },
  { "D",  NOTE_D_v1  },
  { "D#", NOTE_DS_v1 },
  { "E",  NOTE_E_v1  },
  { "F",  NOTE_F_v1  },
  { "F#", NOTE_FS_v1 },
  { "G",  NOTE_G_v1  },
  { "G#", NOTE_GS_v1 },
  { "A",  NOTE_A_v1  },
  { "A#", NOTE_AS_v1 },
  { "B",  NOTE_B_v1  },
};

typedef enum ChordType_v1
{
  CHORD_TYPE_NONE_v1,
  CHORD_TYPE_MAJ_v1,
  CHORD_TYPE_MIN_v1,
  CHORD_TYPE_DIM_v1,
  CHORD_TYPE_SUS4_v1,
  CHORD_TYPE_SUS2_v1,
  CHORD_TYPE_AUG_v1,
  CHORD_TYPE_CUSTOM_v1,
  NUM_CHORD_TYPES_v1,
} ChordType_v1;

static const cyaml_strval_t chord_type_strings_v1[] = {
  { "Invalid", CHORD_TYPE_NONE_v1   },
  { "Maj",     CHORD_TYPE_MAJ_v1    },
  { "min",     CHORD_TYPE_MIN_v1    },
  { "dim",     CHORD_TYPE_DIM_v1    },
  { "sus4",    CHORD_TYPE_SUS4_v1   },
  { "sus2",    CHORD_TYPE_SUS2_v1   },
  { "aug",     CHORD_TYPE_AUG_v1    },
  { "custom",  CHORD_TYPE_CUSTOM_v1 },
};

typedef enum ChordAccent_v1
{
  CHORD_ACC_NONE_v1,
  CHORD_ACC_7_v1,
  CHORD_ACC_j7_v1,
  CHORD_ACC_b9_v1,
  CHORD_ACC_9_v1,
  CHORD_ACC_S9_v1,
  CHORD_ACC_11_v1,
  CHORD_ACC_b5_S11_v1,
  CHORD_ACC_S5_b13_v1,
  CHORD_ACC_6_13_v1,
  NUM_CHORD_ACCENTS_v1,
} ChordAccent_v1;

static const cyaml_strval_t chord_accent_strings_v1[] = {
  { "None",             CHORD_ACC_NONE_v1   },
  { "7",                CHORD_ACC_7_v1      },
  { "j7",               CHORD_ACC_j7_v1     },
  { "\u266D9",          CHORD_ACC_b9_v1     },
  { "9",                CHORD_ACC_9_v1      },
  { "\u266F9",          CHORD_ACC_S9_v1     },
  { "11",               CHORD_ACC_11_v1     },
  { "\u266D5/\u266F11", CHORD_ACC_b5_S11_v1 },
  { "\u266F5/\u266D13", CHORD_ACC_S5_b13_v1 },
  { "6/13",             CHORD_ACC_6_13_v1   },
};

typedef struct ChordDescriptor_v2
{
  int            schema_version;
  bool           has_bass;
  MusicalNote_v1 root_note;
  MusicalNote_v1 bass_note;
  ChordType_v1   type;
  ChordAccent_v1 accent;
  int            notes[48];
  int            inversion;
} ChordDescriptor_v2;

static const cyaml_schema_field_t chord_descriptor_fields_schema_v2[] = {
  YAML_FIELD_INT (ChordDescriptor_v2, schema_version),
  YAML_FIELD_INT (ChordDescriptor_v2, has_bass),
  YAML_FIELD_ENUM (ChordDescriptor_v2, root_note, musical_note_strings_v1),
  YAML_FIELD_ENUM (ChordDescriptor_v2, bass_note, musical_note_strings_v1),
  YAML_FIELD_ENUM (ChordDescriptor_v2, type, chord_type_strings_v1),
  YAML_FIELD_ENUM (ChordDescriptor_v2, accent, chord_accent_strings_v1),
  CYAML_FIELD_SEQUENCE_FIXED (
    "notes",
    CYAML_FLAG_OPTIONAL,
    ChordDescriptor_v2,
    notes,
    &int_schema,
    36),
  YAML_FIELD_INT (ChordDescriptor_v2, inversion),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t chord_descriptor_schema_v2 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordDescriptor_v2,
    chord_descriptor_fields_schema_v2),
};

#endif
