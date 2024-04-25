// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin manager.
 */

#ifndef __PLUGINS_PLUGIN_MANAGER_H__
#define __PLUGINS_PLUGIN_MANAGER_H__

#include "plugins/plugin_descriptor.h"
#include "utils/symap.h"
#include "utils/types.h"

#include "zix/sem.h"

TYPEDEF_STRUCT (CachedPluginDescriptors);
TYPEDEF_STRUCT (PluginCollections);
TYPEDEF_STRUCT (ZCarlaDiscovery);

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_MANAGER (ZRYTHM->plugin_manager)

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

  /** Cached descriptors */
  CachedPluginDescriptors * cached_plugin_descriptors;

  /** Plugin collections. */
  PluginCollections * collections;

  ZCarlaDiscovery * carla_discovery;

  GenericCallback scan_done_cb;
  void *          scan_done_cb_data;

  /** Whether the plugin manager has been set up already. */
  bool setup;

  /** Number of newly scanned (newly cached) plugins. */
  int num_new_plugins;

} PluginManager;

PluginManager *
plugin_manager_new (void);

char **
plugin_manager_get_paths_for_protocol (
  const PluginManager * self,
  const ZPluginProtocol protocol);

char *
plugin_manager_get_paths_for_protocol_separated (
  const PluginManager * self,
  const ZPluginProtocol protocol);

/**
 * Searches in the known paths for this plugin protocol for the given relative
 * path of the plugin and returns the absolute path.
 */
char *
plugin_manager_find_plugin_from_rel_path (
  const PluginManager * self,
  const ZPluginProtocol protocol,
  const char *          rel_path);

void
plugin_manager_begin_scan (
  PluginManager * self,
  const double    max_progress,
  double *        progress,
  GenericCallback cb,
  void *          user_data);

/**
 * Adds a new descriptor.
 */
void
plugin_manager_add_descriptor (PluginManager * self, PluginDescriptor * descr);

/**
 * Updates the text in the greeter.
 */
void
plugin_manager_set_currently_scanning_plugin (
  PluginManager * self,
  const char *    filename,
  const char *    sha1);

/**
 * Returns the PluginDescriptor instance for the
 * given URI.
 *
 * This instance is held by the plugin manager and
 * must not be free'd.
 */
PluginDescriptor *
plugin_manager_find_plugin_from_uri (PluginManager * self, const char * uri);

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
plugin_manager_supports_protocol (PluginManager * self, ZPluginProtocol protocol);

/**
 * Returns an instrument plugin, if any.
 */
PluginDescriptor *
plugin_manager_pick_instrument (PluginManager * self);

void
plugin_manager_clear_plugins (PluginManager * self);

void
plugin_manager_free (PluginManager * self);

/**
 * @}
 */

#endif
