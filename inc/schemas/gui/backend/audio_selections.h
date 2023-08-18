/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Audio selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_AUDIO_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_AUDIO_SELECTIONS_H__

#include "utils/yaml.h"

#include "schemas/dsp/position.h"
#include "schemas/dsp/region_identifier.h"
#include "schemas/gui/backend/arranger_selections.h"

typedef struct AudioSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  bool                  has_selection;
  Position_v1           sel_start;
  Position_v1           sel_end;
  int                   pool_id;
  RegionIdentifier_v1   region_id;
} AudioSelections_v1;

static const cyaml_schema_field_t audio_selections_fields_schema_v1[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections_v1,
    base,
    arranger_selections_fields_schema_v1),
  YAML_FIELD_INT (AudioSelections_v1, schema_version),
  YAML_FIELD_INT (AudioSelections_v1, has_selection),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections_v1,
    sel_start,
    position_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections,
    sel_end,
    position_fields_schema_v1),
  YAML_FIELD_INT (AudioSelections_v1, pool_id),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections_v1,
    region_id,
    region_identifier_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_selections_schema_v1 = {
  YAML_VALUE_PTR (
    AudioSelections_v1,
    audio_selections_fields_schema_v1),
};

#endif
