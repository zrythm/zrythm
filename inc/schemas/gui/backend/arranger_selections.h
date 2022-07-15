/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

#endif
