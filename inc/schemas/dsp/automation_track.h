// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation track schema.
 */

#ifndef __SCHEMAS_AUDIO_AUTOMATION_TRACK_H__
#define __SCHEMAS_AUDIO_AUTOMATION_TRACK_H__

#include "schemas/dsp/automation_point.h"
#include "schemas/dsp/port.h"
#include "schemas/dsp/region.h"

typedef enum AutomationMode_v1
{
  AUTOMATION_MODE_READ_v1,
  AUTOMATION_MODE_RECORD_v1,
  AUTOMATION_MODE_OFF_v1,
  NUM_AUTOMATION_MODES_v1,
} AutomationMode_v1;

static const cyaml_strval_t automation_mode_strings_v1[] = {
  {"Read",       AUTOMATION_MODE_READ_v1  },
  { "Rec",       AUTOMATION_MODE_RECORD_v1},
  { "Off",       AUTOMATION_MODE_OFF_v1   },
  { "<invalid>", NUM_AUTOMATION_MODES_v1  },
};

typedef enum AutomationRecordMode_v1
{
  AUTOMATION_RECORD_MODE_TOUCH_v1,
  AUTOMATION_RECORD_MODE_LATCH_v1,
  NUM_AUTOMATION_RECORD_MODES_v1,
} AutomationRecordMode_v1;

static const cyaml_strval_t automation_record_mode_strings_v1[] = {
  {"Touch",      AUTOMATION_RECORD_MODE_TOUCH_v1},
  { "Latch",     AUTOMATION_RECORD_MODE_LATCH_v1},
  { "<invalid>", NUM_AUTOMATION_RECORD_MODES_v1 },
};

typedef struct AutomationTrack_v1
{
  int                     schema_version;
  int                     index;
  PortIdentifier_v1       port_id;
  bool                    created;
  ZRegion_v1 **           regions;
  int                     num_regions;
  bool                    visible;
  double                  height;
  AutomationMode_v1       automation_mode;
  AutomationRecordMode_v1 record_mode;
} AutomationTrack_v1;

static const cyaml_schema_field_t automation_track_fields_schema_v1[] = {
  YAML_FIELD_INT (AutomationTrack_v1, schema_version),
  YAML_FIELD_INT (AutomationTrack_v1, index),
  YAML_FIELD_MAPPING_EMBEDDED (
    AutomationTrack_v1,
    port_id,
    port_identifier_fields_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    AutomationTrack_v1,
    regions,
    region_schema_v1),
  YAML_FIELD_INT (AutomationTrack_v1, created),
  YAML_FIELD_ENUM (AutomationTrack_v1, automation_mode, automation_mode_strings_v1),
  CYAML_FIELD_ENUM (
    "record_mode",
    CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
    AutomationTrack_v1,
    record_mode,
    automation_record_mode_strings_v1,
    CYAML_ARRAY_LEN (automation_record_mode_strings_v1)),
  YAML_FIELD_INT (AutomationTrack_v1, visible),
  YAML_FIELD_FLOAT (AutomationTrack_v1, height),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t automation_track_schema_v1 = {
  YAML_VALUE_PTR (AutomationTrack_v1, automation_track_fields_schema_v1),
};

#endif
