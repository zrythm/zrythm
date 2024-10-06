// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Automation tracklist schema.
 */

#ifndef __SCHEMAS_AUDIO_AUTOMATION_TRACKLIST_H__
#define __SCHEMAS_AUDIO_AUTOMATION_TRACKLIST_H__

#include "common/utils/yaml.h"
#include "gui/backend/backend/cyaml_schemas/dsp/automation_track.h"

typedef struct AutomationTracklist_v1
{
  int                   schema_version;
  AutomationTrack_v1 ** ats;
  int                   num_ats;
} AutomationTracklist_v1;

static const cyaml_schema_field_t automation_tracklist_fields_schema_v1[] = {
  YAML_FIELD_INT (AutomationTracklist_v1, schema_version),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    AutomationTracklist_v1,
    ats,
    automation_track_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t automation_tracklist_schema_v1 = {
  YAML_VALUE_PTR (AutomationTracklist_v1, automation_tracklist_fields_schema_v1),
};

#endif
