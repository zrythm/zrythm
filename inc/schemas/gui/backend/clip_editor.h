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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Clip editor schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_CLIP_EDITOR_H__
#define __SCHEMAS_GUI_BACKEND_CLIP_EDITOR_H__

#include <stdbool.h>

#include "audio/region_identifier.h"
#include "gui/backend/audio_clip_editor.h"
#include "gui/backend/automation_editor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/piano_roll.h"

typedef struct ClipEditor_v1
{
  int                 schema_version;
  RegionIdentifier_v1 region_id;
  bool                has_region;
  PianoRoll_v1        piano_roll;
  AudioClipEditor_v1  audio_clip_editor;
  AutomationEditor_v1 automation_editor;
  ChordEditor_v1      chord_editor;
  int                 region_changed;
} ClipEditor_v1;

static const cyaml_schema_field_t clip_editor_fields_schema_v1[] = {
  YAML_FIELD_INT (ClipEditor_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor_v1,
    region_id,
    region_identifier_fields_schema_v1),
  YAML_FIELD_INT (ClipEditor_v1, has_region),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor_v1,
    piano_roll,
    piano_roll_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor_v1,
    automation_editor,
    automation_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor_v1,
    chord_editor,
    chord_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor_v1,
    audio_clip_editor,
    audio_clip_editor_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t clip_editor_schema_v1 = {
  YAML_VALUE_PTR (ClipEditor_v1, clip_editor_fields_schema_v1),
};

#endif
