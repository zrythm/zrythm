/*
 * SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Chord selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_CHORD_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_CHORD_SELECTIONS_H__

#include "gui/backend/backend/cyaml_schemas/dsp/chord_object.h"
#include "gui/backend/backend/cyaml_schemas/gui/backend/arranger_selections.h"
#include "utils/yaml.h"

typedef struct ChordSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  ChordObject_v1 **     chord_objects;
  int                   num_chord_objects;
  size_t                chord_objects_size;
} ChordSelections_v1;

static const cyaml_schema_field_t chord_selections_fields_schema_v1[] = {
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
  YAML_VALUE_PTR (ChordSelections_v1, chord_selections_fields_schema_v1),
};

#endif
