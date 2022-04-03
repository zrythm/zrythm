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
 * Plugin collection schema.
 */

#ifndef __SCHEMAS_PLUGINS_PLUGIN_COLLECTIONS_H__
#define __SCHEMAS_PLUGINS_PLUGIN_COLLECTIONS_H__

#include "utils/yaml.h"

#include "schemas/plugins/collection.h"

typedef struct PluginCollections_v1
{
  int                   schema_version;
  PluginCollection_v1 * collections[90000];
  int                   num_collections;
} PluginCollections;

static const cyaml_schema_field_t
  plugin_collections_fields_schema_v1[] = {
    YAML_FIELD_INT (
      PluginCollections_v1,
      schema_version),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      PluginCollections_v1,
      collections,
      plugin_collection_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  plugin_collections_schema_v1 = {
    YAML_VALUE_PTR (
      PluginCollections_v1,
      plugin_collections_fields_schema_v1),
  };

#endif
