// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

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

/**
 * Plugin collection used in the plugin browser.
 */
typedef struct PluginCollection
{
  /** Name of the collection. */
  char * name;

  /** Description of the collection (optional). */
  char * description;

  /** Plugin descriptors. */
  GPtrArray * descriptors;
} PluginCollection;

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
plugin_collection_set_name (PluginCollection * self, const char * name);

/**
 * Returns whether the collection contains the
 * given descriptor.
 *
 * @param match_pointer Whether to check pointers
 *   or the descriptor details.
 */
NONNULL bool
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
 * @memberof PluginCollection
 */
GMenuModel *
plugin_collection_generate_context_menu (const PluginCollection * self);

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
