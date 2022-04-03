/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Automation point schema.
 */

#ifndef __SCHEMAS_AUDIO_AUTOMATION_POINT_H__
#define __SCHEMAS_AUDIO_AUTOMATION_POINT_H__

#include "schemas/audio/curve.h"
#include "schemas/gui/backend/arranger_object.h"

typedef struct AutomationPoint_v1
{
  ArrangerObject_v1 base;
  int               schema_version;
  float             fvalue;
  float             normalized_val;
  CurveOptions_v1   curve_opts;
  int               index;
} AutomationPoint_v1;

static const cyaml_schema_field_t
  automation_point_fields_schema_v1[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      AutomationPoint_v1,
      base,
      arranger_object_fields_schema_v1),
    YAML_FIELD_INT (AutomationPoint_v1, schema_version),
    YAML_FIELD_FLOAT (AutomationPoint_v1, fvalue),
    YAML_FIELD_FLOAT (
      AutomationPoint_v1,
      normalized_val),
    YAML_FIELD_INT (AutomationPoint_v1, index),
    YAML_FIELD_MAPPING_EMBEDDED (
      AutomationPoint_v1,
      curve_opts,
      curve_options_fields_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  automation_point_schema_v1 = {
    YAML_VALUE_PTR (
      AutomationPoint_v1,
      automation_point_fields_schema_v1),
  };

#endif
