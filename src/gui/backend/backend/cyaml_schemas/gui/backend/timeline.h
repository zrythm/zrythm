// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SCHEMAS_GUI_BACKEND_TIMELINE_H__
#define __SCHEMAS_GUI_BACKEND_TIMELINE_H__

#include "common/utils/yaml.h"
#include "gui/backend/backend/cyaml_schemas/gui/backend/editor_settings.h"

typedef struct Timeline_v1
{
  int schema_version;

  /** Settings for the timeline. */
  EditorSettings_v1 editor_settings;
} Timeline_v1;

static const cyaml_schema_field_t timeline_fields_schema_v1[] = {
  YAML_FIELD_INT (Timeline_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    Timeline_v1,
    editor_settings,
    editor_settings_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t timeline_schema_v1 = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER, Timeline_v1, timeline_fields_schema_v1),
};

#endif
