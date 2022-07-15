/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Editor settings schema.
 */

#ifndef __SCHEMAS_GUI_BACKEND_EDITOR_SETTINGS_H__
#define __SCHEMAS_GUI_BACKEND_EDITOR_SETTINGS_H__

#include "utils/yaml.h"

typedef struct EditorSettings_v1
{
  int    schema_version;
  int    scroll_start_x;
  int    scroll_start_y;
  double hzoom_level;
} EditorSettings_v1;

static const cyaml_schema_field_t
  editor_settings_fields_schema_v1[] = {
    YAML_FIELD_INT (EditorSettings_v1, schema_version),
    YAML_FIELD_INT (EditorSettings_v1, scroll_start_x),
    YAML_FIELD_INT (EditorSettings_v1, scroll_start_y),
    YAML_FIELD_FLOAT (EditorSettings_v1, hzoom_level),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t editor_settings_schema_v1 = {
  YAML_VALUE_PTR (
    EditorSettings_v1,
    editor_settings_fields_schema_v1),
};

#endif
