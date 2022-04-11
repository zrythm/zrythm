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
 * Velocity schema.
 */

#ifndef __SCHEMAS_AUDIO_VELOCITY_H__
#define __SCHEMAS_AUDIO_VELOCITY_H__

#include "schemas/gui/backend/arranger_object.h"

typedef struct Velocity_v0
{
  ArrangerObject_v1 base;
  uint8_t           vel;
} Velocity_v0;

static const cyaml_schema_field_t
  velocity_fields_schema_v0[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      Velocity_v0,
      base,
      arranger_object_fields_schema_v1),
    YAML_FIELD_UINT (Velocity_v0, vel),
    CYAML_FIELD_END
  };

static const cyaml_schema_value_t velocity_schema_v0 = {
  YAML_VALUE_PTR (
    Velocity_v0,
    velocity_fields_schema_v0),
};

typedef struct Velocity_v1
{
  ArrangerObject_v1 base;
  int               schema_version;
  uint8_t           vel;
} Velocity_v1;

static const cyaml_schema_field_t
  velocity_fields_schema_v1[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      Velocity_v0,
      base,
      arranger_object_fields_schema_v1),
    YAML_FIELD_INT (Velocity_v0, schema_version),
    YAML_FIELD_UINT (Velocity_v0, vel),
    CYAML_FIELD_END
  };

static const cyaml_schema_value_t velocity_schema_v1 = {
  YAML_VALUE_PTR (
    Velocity_v1,
    velocity_fields_schema_v1),
};

Velocity_v1 *
velocity_migrate_v0_1 (Velocity_v0 * vel);

#endif
