// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

static const cyaml_schema_field_t editor_settings_fields_schema[] = {
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

void
editor_settings_set_scroll_start_x (EditorSettings * self, int x, bool validate);

void
editor_settings_set_scroll_start_y (EditorSettings * self, int y, bool validate);

/**
 * Appends the given deltas to the scroll x/y values.
 */
void
editor_settings_append_scroll (
  EditorSettings * self,
  int              dx,
  int              dy,
  bool             validate);

/**
 * @}
 */

#endif
