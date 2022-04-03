/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin manager.
 */

#ifndef __PLUGINS_PLUGIN_MANAGER_H__
#define __PLUGINS_PLUGIN_MANAGER_H__

#include "plugins/lv2/lv2_urid.h"
#include "plugins/plugin_descriptor.h"
#include "utils/symap.h"

#include "zix/sem.h"
#include <lilv/lilv.h>

typedef struct CachedPluginDescriptors
  CachedPluginDescriptors;
typedef struct PluginCollections PluginCollections;

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_MANAGER (ZRYTHM->plugin_manager)
#define LILV_WORLD (PLUGIN_MANAGER->lilv_world)
#define LILV_PLUGINS (PLUGIN_MANAGER->lilv_plugins)
#define LV2_GENERATOR_PLUGIN "Generator"
#define LV2_CONSTANT_PLUGIN "Constant"
#define LV2_INSTRUMENT_PLUGIN "Instrument"
#define LV2_OSCILLATOR_PLUGIN "Oscillator"
#define PM_URIDS (PLUGIN_MANAGER->urids)
#define PM_SYMAP (PLUGIN_MANAGER->symap)
#define PM_SYMAP_LOCK (PLUGIN_MANAGER->symap_lock)
#define PM_GET_NODE(uri) \
  plugin_manager_get_node (PLUGIN_MANAGER, uri)

typedef struct PluginDescriptor PluginDescriptor;

/**
 * The PluginManager is responsible for scanning
 * and keeping track of available Plugin's.
 */
typedef struct PluginManager
{
  /**
   * Scanned plugin descriptors.
   */
  GPtrArray * plugin_descriptors;

  /** Plugin categories. */
  char * plugin_categories[500];
  int    num_plugin_categories;

  /** Plugin authors. */
  char * plugin_authors[6000];
  int    num_plugin_authors;

  LilvWorld *         lilv_world;
  const LilvPlugins * lilv_plugins;

  LilvNode ** nodes;
  int         num_nodes;
  size_t      nodes_size;

  /** Cached VST descriptors */
  CachedPluginDescriptors * cached_plugin_descriptors;

  /** Plugin collections. */
  PluginCollections * collections;

  /** URI map for URID feature. */
  Symap * symap;
  /** Lock for URI map. */
  ZixSem symap_lock;

  /** URIDs. */
  Lv2URIDs urids;

  char * lv2_path;

  /** Whether the plugin manager has been set up
   * already. */
  bool setup;

} PluginManager;

PluginManager *
plugin_manager_new (void);

/**
 * Returns a cached LilvNode for the given URI.
 *
 * If a node doesn't exist for the given URI, a
 * node is created and cached.
 */
const LilvNode *
plugin_manager_get_node (
  PluginManager * self,
  const char *    uri);

/**
 * Scans for plugins, optionally updating the
 * progress.
 *
 * @param max_progress Maximum progress for this
 *   stage.
 * @param progress Pointer to a double (0.0-1.0) to
 *   update based on the current progress.
 */
void
plugin_manager_scan_plugins (
  PluginManager * self,
  const double    max_progress,
  double *        progress);

/**
 * Returns the PluginDescriptor instance for the
 * given URI.
 *
 * This instance is held by the plugin manager and
 * must not be free'd.
 */
PluginDescriptor *
plugin_manager_find_plugin_from_uri (
  PluginManager * self,
  const char *    uri);

/**
 * Finds and returns the PluginDescriptor instance
 * matching the given descriptor.
 *
 * This instance is held by the plugin manager and
 * must not be free'd.
 */
PluginDescriptor *
plugin_manager_find_from_descriptor (
  PluginManager *          self,
  const PluginDescriptor * src_descr);

/**
 * Returns if the plugin manager supports the given
 * plugin protocol.
 */
bool
plugin_manager_supports_protocol (
  PluginManager * self,
  PluginProtocol  protocol);

/**
 * Returns an instrument plugin, if any.
 */
PluginDescriptor *
plugin_manager_pick_instrument (
  PluginManager * self);

void
plugin_manager_clear_plugins (PluginManager * self);

void
plugin_manager_free (PluginManager * self);

/**
 * @}
 */

#endif
