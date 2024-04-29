// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

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

#define PLUGIN_COLLECTIONS_SCHEMA_VERSION 2

/**
 * Serializable plugin collections.
 */
typedef struct PluginCollections
{
  /** Plugin collections. */
  GPtrArray * collections;
} PluginCollections;

/**
 * Reads the file and fills up the object.
 */
PluginCollections *
plugin_collections_read_or_new (void);

void
plugin_collections_serialize_to_file (PluginCollections * self);

/**
 * Appends a collection.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 *
 * @memberof PluginCollections
 */
void
plugin_collections_add (
  PluginCollections *      self,
  const PluginCollection * collection,
  bool                     serialize);

/**
 * @memberof PluginCollections
 */
const PluginCollection *
plugin_collections_find_from_name (
  const PluginCollections * self,
  const char *              name);

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
