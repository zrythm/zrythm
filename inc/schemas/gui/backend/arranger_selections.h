// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * ArrangerSelections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_ARRANGER_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_ARRANGER_SELECTIONS_H__

#include <stdbool.h>

#include "utils/yaml.h"

typedef enum ArrangerSelectionsType_v1
{
  ARRANGER_SELECTIONS_TYPE_NONE_v1,
  ARRANGER_SELECTIONS_TYPE_CHORD_v1,
  ARRANGER_SELECTIONS_TYPE_TIMELINE_v1,
  ARRANGER_SELECTIONS_TYPE_MIDI_v1,
  ARRANGER_SELECTIONS_TYPE_AUTOMATION_v1,
  ARRANGER_SELECTIONS_TYPE_AUDIO_v1,
} ArrangerSelectionsType_v1;

static const cyaml_strval_t
  arranger_selections_type_strings_v1[] = {
    {"Chord",       ARRANGER_SELECTIONS_TYPE_CHORD_v1     },
    { "Timeline",   ARRANGER_SELECTIONS_TYPE_TIMELINE_v1  },
    { "MIDI",       ARRANGER_SELECTIONS_TYPE_MIDI_v1      },
    { "Automation", ARRANGER_SELECTIONS_TYPE_AUTOMATION_v1},
    { "Audio",      ARRANGER_SELECTIONS_TYPE_AUDIO_v1     },
};

typedef struct ArrangerSelections_v1
{
  int                       schema_version;
  ArrangerSelectionsType_v1 type;
  int                       magic;
} ArrangerSelections_v1;

static const cyaml_schema_field_t
  arranger_selections_fields_schema_v1[] = {
    YAML_FIELD_INT (ArrangerSelections_v1, schema_version),
    YAML_FIELD_ENUM (
      ArrangerSelections_v1,
      type,
      arranger_selections_type_strings_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t arranger_selections_schema_v1 = {
  YAML_VALUE_PTR (
    ArrangerSelections_v1,
    arranger_selections_fields_schema_v1),
};

ArrangerSelections *
arranger_selections_upgrade_from_v1 (
  ArrangerSelections_v1 * old);

#endif
