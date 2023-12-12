// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Clip editor schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_CLIP_EDITOR_H__
#define __SCHEMAS_GUI_BACKEND_CLIP_EDITOR_H__

#include <stdbool.h>

#include "schemas/dsp/region_identifier.h"
#include "schemas/gui/backend/audio_clip_editor.h"
#include "schemas/gui/backend/automation_editor.h"
#include "schemas/gui/backend/chord_editor.h"
#include "schemas/gui/backend/piano_roll.h"

typedef struct ClipEditor_v1
{
  int                   schema_version;
  RegionIdentifier_v1   region_id;
  bool                  has_region;
  PianoRoll_v1 *        piano_roll;
  AudioClipEditor_v1 *  audio_clip_editor;
  AutomationEditor_v1 * automation_editor;
  ChordEditor_v1 *      chord_editor;
  int                   region_changed;
} ClipEditor_v1;

static const cyaml_schema_field_t clip_editor_fields_schema_v1[] = {
  YAML_FIELD_INT (ClipEditor_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor_v1,
    region_id,
    region_identifier_fields_schema_v1),
  YAML_FIELD_INT (ClipEditor_v1, has_region),
  YAML_FIELD_MAPPING_PTR (ClipEditor_v1, piano_roll, piano_roll_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor_v1,
    automation_editor,
    automation_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor_v1,
    chord_editor,
    chord_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor_v1,
    audio_clip_editor,
    audio_clip_editor_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t clip_editor_schema_v1 = {
  YAML_VALUE_PTR (ClipEditor_v1, clip_editor_fields_schema_v1),
};

#endif
