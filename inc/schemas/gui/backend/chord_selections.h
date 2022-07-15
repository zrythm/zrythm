/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Chord selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_CHORD_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_CHORD_SELECTIONS_H__

#include "utils/yaml.h"

#include "schemas/audio/chord_object.h"
#include "schemas/gui/backend/arranger_selections.h"

typedef struct ChordSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  ChordObject_v1 **     chord_objects;
  int                   num_chord_objects;
  size_t                chord_objects_size;
} ChordSelections_v1;

static const cyaml_schema_field_t
  chord_selections_fields_schema_v1[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      ChordSelections_v1,
      base,
      arranger_selections_fields_schema_v1),
    YAML_FIELD_INT (ChordSelections_v1, schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      ChordSelections_v1,
      chord_objects,
      chord_object_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t chord_selections_schema_v1 = {
  YAML_VALUE_PTR (
    ChordSelections_v1,
    chord_selections_fields_schema_v1),
};

#endif
