// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_AUDIO_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_AUDIO_SELECTIONS_H__

#include "schemas/dsp/position.h"
#include "schemas/dsp/region_identifier.h"
#include "schemas/gui/backend/arranger_selections.h"
#include "utils/yaml.h"

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
    AudioSelections_v1,
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
  YAML_VALUE_PTR (AudioSelections_v1, audio_selections_fields_schema_v1),
};

#endif
