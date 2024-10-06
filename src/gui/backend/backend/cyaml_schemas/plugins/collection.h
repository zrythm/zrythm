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

#ifndef __SCHEMAS_PLUGINS_PLUGIN_COLLECTION_H__
#define __SCHEMAS_PLUGINS_PLUGIN_COLLECTION_H__

#include "common/utils/yaml.h"
#include "gui/backend/backend/cyaml_schemas/plugins/plugin_descriptor.h"

typedef struct PluginCollection_v1
{
  int                    schema_version;
  char *                 name;
  char *                 description;
  PluginDescriptor_v1 ** descriptors;
  int                    num_descriptors;
  size_t                 descriptors_size;
} PluginCollection_v1;

static const cyaml_schema_field_t plugin_collection_fields_schema_v1[] = {
  YAML_FIELD_INT (PluginCollection_v1, schema_version),
  YAML_FIELD_STRING_PTR (PluginCollection_v1, name),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginCollection_v1, description),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    PluginCollection_v1,
    descriptors,
    plugin_descriptor_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_collection_schema_v1 = {
  YAML_VALUE_PTR (PluginCollection_v1, plugin_collection_fields_schema_v1),
};

#endif
