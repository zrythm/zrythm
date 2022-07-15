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
 * Plugin collection.
 */

#ifndef __PLUGINS_PLUGIN_COLLECTION_H__
#define __PLUGINS_PLUGIN_COLLECTION_H__

#include "plugins/plugin_descriptor.h"
#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_COLLECTION_SCHEMA_VERSION 1

/**
 * Plugin collection used in the plugin browser.
 */
typedef struct PluginCollection
{
  int schema_version;

  /** Name of the collection. */
  char * name;

  /** Description of the collection (optional). */
  char * description;

  /** Plugin descriptors. */
  PluginDescriptor ** descriptors;
  int                 num_descriptors;
  size_t              descriptors_size;
} PluginCollection;

static const cyaml_schema_field_t
  plugin_collection_fields_schema[] = {
    YAML_FIELD_INT (PluginCollection, schema_version),
    YAML_FIELD_STRING_PTR (PluginCollection, name),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      PluginCollection,
      description),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
      PluginCollection,
      descriptors,
      plugin_descriptor_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t plugin_collection_schema = {
  YAML_VALUE_PTR (
    PluginCollection,
    plugin_collection_fields_schema),
};

void
plugin_collection_init_loaded (PluginCollection * self);

/**
 * Creates a new plugin collection.
 */
PluginCollection *
plugin_collection_new (void);

/**
 * Clones a plugin collection.
 */
PluginCollection *
plugin_collection_clone (const PluginCollection * self);

char *
plugin_collection_get_name (PluginCollection * self);

void
plugin_collection_set_name (
  PluginCollection * self,
  const char *       name);

/**
 * Returns whether the collection contains the
 * given descriptor.
 *
 * @param match_pointer Whether to check pointers
 *   or the descriptor details.
 */
bool
plugin_collection_contains_descriptor (
  PluginCollection *       self,
  const PluginDescriptor * descr,
  bool                     match_pointer);

/**
 * Appends a descriptor to the collection.
 */
void
plugin_collection_add_descriptor (
  PluginCollection *       self,
  const PluginDescriptor * descr);

/**
 * Removes the descriptor matching the given one from
 * the collection.
 */
void
plugin_collection_remove_descriptor (
  PluginCollection *       self,
  const PluginDescriptor * descr);

/**
 * Removes all the descriptors.
 */
void
plugin_collection_clear (PluginCollection * self);

void
plugin_collection_free (PluginCollection * self);

/**
 * @}
 */

#endif
