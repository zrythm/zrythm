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
 * API for selections in the AutomationArrangerWidget.
 */

#ifndef __GUI_BACKEND_AUTOMATION_SELECTIONS_H__
#define __GUI_BACKEND_AUTOMATION_SELECTIONS_H__

#include "audio/automation_point.h"
#include "gui/backend/arranger_selections.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUTOMATION_SELECTIONS_SCHEMA_VERSION 1

#define AUTOMATION_SELECTIONS (PROJECT->automation_selections)

/**
 * Selections to be used for the AutomationArrangerWidget's
 * current selections, copying, undoing, etc.
 */
typedef struct AutomationSelections
{
  int schema_version;

  ArrangerSelections base;

  /** Selected AutomationObject's. */
  AutomationPoint ** automation_points;
  int                num_automation_points;
  size_t             automation_points_size;

} AutomationSelections;

static const cyaml_schema_field_t
  automation_selections_fields_schema[] = {
    YAML_FIELD_INT (AutomationSelections, schema_version),
    YAML_FIELD_MAPPING_EMBEDDED (
      AutomationSelections,
      base,
      arranger_selections_fields_schema),
    CYAML_FIELD_SEQUENCE_COUNT (
      "automation_points",
      CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
      AutomationSelections,
      automation_points,
      num_automation_points,
      &automation_point_schema,
      0,
      CYAML_UNLIMITED),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t automation_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationSelections,
    automation_selections_fields_schema),
};

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
bool
automation_selections_can_be_pasted (
  AutomationSelections * ts,
  Position *             pos,
  ZRegion *              r);

/**
* @}
*/

#endif
