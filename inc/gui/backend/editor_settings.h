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
 * Common editor settings.
 */

#ifndef __GUI_BACKEND_EDITOR_SETTINGS_H__
#define __GUI_BACKEND_EDITOR_SETTINGS_H__

#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define EDITOR_SETTINGS_SCHEMA_VERSION 1

/**
 * Common editor settings.
 */
typedef struct EditorSettings
{
  int schema_version;

  /** Horizontal scroll start position. */
  int scroll_start_x;

  /** Vertical scroll start position. */
  int scroll_start_y;

  /** Horizontal zoom level. */
  double hzoom_level;
} EditorSettings;

static const cyaml_schema_field_t
  editor_settings_fields_schema[] = {
    YAML_FIELD_INT (EditorSettings, schema_version),
    YAML_FIELD_INT (EditorSettings, scroll_start_x),
    YAML_FIELD_INT (EditorSettings, scroll_start_y),
    YAML_FIELD_FLOAT (EditorSettings, hzoom_level),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t editor_settings_schema = {
  YAML_VALUE_PTR (EditorSettings, editor_settings_fields_schema),
};

void
editor_settings_init (EditorSettings * self);

/**
 * @}
 */

#endif
