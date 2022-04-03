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
 * Plugin collections.
 */

#ifndef __PLUGINS_PLUGIN_COLLECTIONS_H__
#define __PLUGINS_PLUGIN_COLLECTIONS_H__

#include "plugins/collection.h"
#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_COLLECTIONS_SCHEMA_VERSION 1

/**
 * Serializable plugin collections.
 */
typedef struct PluginCollections
{
  /** Version of the file. */
  int schema_version;

  /** Plugin collections. */
  PluginCollection * collections[90000];
  int                num_collections;
} PluginCollections;

static const cyaml_schema_field_t
  plugin_collections_fields_schema[] = {
    YAML_FIELD_INT (PluginCollections, schema_version),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      PluginCollections,
      collections,
      plugin_collection_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  plugin_collections_schema = {
    YAML_VALUE_PTR (
      PluginCollections,
      plugin_collections_fields_schema),
  };

/**
 * Reads the file and fills up the object.
 */
PluginCollections *
plugin_collections_new (void);

void
plugin_collections_serialize_to_file (
  PluginCollections * self);

/**
 * Appends a collection.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
plugin_collections_add (
  PluginCollections *      self,
  const PluginCollection * collection,
  bool                     serialize);

/**
 * Removes the given collection.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
plugin_collections_remove (
  PluginCollections * self,
  PluginCollection *  collection,
  bool                serialize);

void
plugin_collections_free (PluginCollections * self);

/**
 * @}
 */

#endif
