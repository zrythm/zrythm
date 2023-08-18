// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Automation point schema.
 */

#ifndef __SCHEMAS_AUDIO_AUTOMATION_POINT_H__
#define __SCHEMAS_AUDIO_AUTOMATION_POINT_H__

#include "schemas/dsp/curve.h"
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
    YAML_FIELD_FLOAT (AutomationPoint_v1, normalized_val),
    YAML_FIELD_INT (AutomationPoint_v1, index),
    YAML_FIELD_MAPPING_EMBEDDED (
      AutomationPoint_v1,
      curve_opts,
      curve_options_fields_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t automation_point_schema_v1 = {
  YAML_VALUE_PTR (
    AutomationPoint_v1,
    automation_point_fields_schema_v1),
};

AutomationPoint *
automation_point_upgrade_from_v1 (AutomationPoint_v1 * old);

#endif
