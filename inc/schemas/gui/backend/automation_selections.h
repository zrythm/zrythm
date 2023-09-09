// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation selections schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_AUTOMATION_SELECTIONS_H__
#define __SCHEMAS_GUI_BACKEND_AUTOMATION_SELECTIONS_H__

#include "dsp/automation_point.h"
#include "gui/backend/arranger_selections.h"
#include "utils/yaml.h"

typedef struct AutomationSelections_v1
{
  ArrangerSelections_v1 base;
  int                   schema_version;
  AutomationPoint_v1 ** automation_points;
  int                   num_automation_points;
  size_t                automation_points_size;
} AutomationSelections_v1;

static const cyaml_schema_field_t automation_selections_fields_schema_v1[] = {
  YAML_FIELD_INT (AutomationSelections_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    AutomationSelections_v1,
    base,
    arranger_selections_fields_schema_v1),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automation_points",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    AutomationSelections_v1,
    automation_points,
    num_automation_points,
    &automation_point_schema_v1,
    0,
    CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t automation_selections_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationSelections_v1,
    automation_selections_fields_schema_v1),
};

#endif
