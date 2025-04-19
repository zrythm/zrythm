/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Plugin collection schema.
 */

#ifndef __SCHEMAS_PLUGINS_PLUGIN_COLLECTIONS_H__
#define __SCHEMAS_PLUGINS_PLUGIN_COLLECTIONS_H__

#include "gui/backend/backend/cyaml_schemas/plugins/collection.h"
#include "utils/yaml.h"

typedef struct PluginCollections_v1
{
  int                   schema_version;
  PluginCollection_v1 * collections[90000];
  int                   num_collections;
} PluginCollections;

static const cyaml_schema_field_t plugin_collections_fields_schema_v1[] = {
  YAML_FIELD_INT (PluginCollections_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    PluginCollections_v1,
    collections,
    plugin_collection_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_collections_schema_v1 = {
  YAML_VALUE_PTR (PluginCollections_v1, plugin_collections_fields_schema_v1),
};

#endif
