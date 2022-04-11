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
 * @file
 *
 * Automation track schema.
 */

#ifndef __SCHEMAS_AUDIO_AUTOMATION_TRACK_H__
#define __SCHEMAS_AUDIO_AUTOMATION_TRACK_H__

#include "schemas/audio/automation_point.h"
#include "schemas/audio/port.h"
#include "schemas/audio/region.h"

typedef enum AutomationMode_v1
{
  AUTOMATION_MODE_READ_v1,
  AUTOMATION_MODE_RECORD_v1,
  AUTOMATION_MODE_OFF_v1,
  NUM_AUTOMATION_MODES_v1,
} AutomationMode_v1;

static const cyaml_strval_t
  automation_mode_strings_v1[] = {
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

static const cyaml_strval_t
  automation_record_mode_strings_v1[] = {
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
  size_t                  regions_size;
  bool                    visible;
  int                     y;
  double                  height;
  float                   last_recorded_value;
  AutomationMode_v1       automation_mode;
  AutomationRecordMode_v1 record_mode;
  bool                    recording_started;
  void *                  recording_region;
  bool                    recording_start_sent;
  bool                    recording_paused;
  void *                  top_right_buttons[8];
  int                     num_top_right_buttons;
  void *                  top_left_buttons[8];
  int                     num_top_left_buttons;
  void *                  bot_right_buttons[8];
  int                     num_bot_right_buttons;
  void *                  am_widget;
  void *                  bot_left_buttons[8];
  int                     num_bot_left_buttons;
} AutomationTrack_v1;

static const cyaml_schema_field_t
  automation_track_fields_schema_v1[] = {
    YAML_FIELD_INT (
      AutomationTrack_v1,
      schema_version),
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
    YAML_FIELD_ENUM (
      AutomationTrack_v1,
      automation_mode,
      automation_mode_strings_v1),
    YAML_FIELD_INT (AutomationTrack_v1, visible),
    YAML_FIELD_FLOAT (AutomationTrack_v1, height),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  automation_track_schema_v1 = {
    YAML_VALUE_PTR (
      AutomationTrack_v1,
      automation_track_fields_schema_v1),
  };

#endif
