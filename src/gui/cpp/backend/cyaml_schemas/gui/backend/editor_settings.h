// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Editor settings schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_EDITOR_SETTINGS_H__
#define __SCHEMAS_GUI_BACKEND_EDITOR_SETTINGS_H__

#include "common/utils/yaml.h"

typedef struct EditorSettings_v1
{
  int    schema_version;
  int    scroll_start_x;
  int    scroll_start_y;
  double hzoom_level;
} EditorSettings_v1;

static const cyaml_schema_field_t editor_settings_fields_schema_v1[] = {
  YAML_FIELD_INT (EditorSettings_v1, schema_version),
  YAML_FIELD_INT (EditorSettings_v1, scroll_start_x),
  YAML_FIELD_INT (EditorSettings_v1, scroll_start_y),
  YAML_FIELD_FLOAT (EditorSettings_v1, hzoom_level),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t editor_settings_schema_v1 = {
  YAML_VALUE_PTR (EditorSettings_v1, editor_settings_fields_schema_v1),
};

#endif
