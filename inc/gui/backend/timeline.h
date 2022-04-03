/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Timeline backend.
 */

#ifndef __GUI_BACKEND_TIMELINE_H__
#define __GUI_BACKEND_TIMELINE_H__

#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define TIMELINE_SCHEMA_VERSION 1

#define PRJ_TIMELINE (PROJECT->timeline)

/**
 * Clip editor serializable backend.
 *
 * The actual widgets should reflect the
 * information here.
 */
typedef struct Timeline
{
  int schema_version;

  /** Settings for the timeline. */
  EditorSettings editor_settings;
} Timeline;

static const cyaml_schema_field_t
  timeline_fields_schema[] = {
    YAML_FIELD_INT (Timeline, schema_version),
    YAML_FIELD_MAPPING_EMBEDDED (
      Timeline,
      editor_settings,
      editor_settings_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t timeline_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Timeline,
    timeline_fields_schema),
};

/**
 * Inits the Timeline after a Project is loaded.
 */
void
timeline_init_loaded (Timeline * self);

/**
 * Inits the Timeline instance.
 */
void
timeline_init (Timeline * self);

Timeline *
timeline_clone (Timeline * src);

/**
 * Creates a new Timeline instance.
 */
Timeline *
timeline_new (void);

void
timeline_free (Timeline * self);

/**
 * @}
 */

#endif
