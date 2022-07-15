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

#ifndef __SCHEMAS_PLUGINS_PLUGIN_COLLECTION_H__
#define __SCHEMAS_PLUGINS_PLUGIN_COLLECTION_H__

#include "utils/yaml.h"

#include "schemas/plugins/plugin_descriptor.h"

typedef struct PluginCollection_v1
{
  int                    schema_version;
  char *                 name;
  char *                 description;
  PluginDescriptor_v1 ** descriptors;
  int                    num_descriptors;
  size_t                 descriptors_size;
} PluginCollection_v1;

static const cyaml_schema_field_t
  plugin_collection_fields_schema_v1[] = {
    YAML_FIELD_INT (PluginCollection_v1, schema_version),
    YAML_FIELD_STRING_PTR (PluginCollection_v1, name),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      PluginCollection_v1,
      description),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
      PluginCollection_v1,
      descriptors,
      plugin_descriptor_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t plugin_collection_schema_v1 = {
  YAML_VALUE_PTR (
    PluginCollection_v1,
    plugin_collection_fields_schema_v1),
};

#endif
