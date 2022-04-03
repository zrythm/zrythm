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
 * Automation tracklist schema.
 */

#ifndef __SCHEMAS_AUDIO_AUTOMATION_TRACKLIST_H__
#define __SCHEMAS_AUDIO_AUTOMATION_TRACKLIST_H__

#include "utils/yaml.h"

#include "schemas/audio/automation_track.h"

typedef struct AutomationTracklist_v1
{
  int                   schema_version;
  AutomationTrack_v1 ** ats;
  int                   num_ats;
  size_t                ats_size;
  int                   track_pos;
} AutomationTracklist_v1;

static const cyaml_schema_field_t
  automation_tracklist_fields_schema_v1[] = {
    YAML_FIELD_INT (
      AutomationTracklist_v1,
      schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      AutomationTracklist_v1,
      ats,
      automation_track_schema_v1),
    YAML_FIELD_INT (AutomationTracklist_v1, track_pos),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  automation_tracklist_schema_v1 = {
    YAML_VALUE_PTR (
      AutomationTracklist_v1,
      automation_tracklist_fields_schema_v1),
  };

#endif
