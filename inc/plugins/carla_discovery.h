// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Carla discovery.
 */

#ifndef __PLUGINS_CARLA_DISCOVERY_H__
#define __PLUGINS_CARLA_DISCOVERY_H__

#include "zrythm-config.h"

#ifdef HAVE_CARLA

#  include "plugins/plugin_descriptor.h"
#  include "settings/plugin_settings.h"
#  include "utils/types.h"

#  include <CarlaUtils.h>

TYPEDEF_STRUCT (PluginDescriptor);
TYPEDEF_STRUCT (PluginManager);

/**
 * @addtogroup plugins
 *
 * @{
 */

typedef struct ZCarlaDiscovery
{
  /**
   * Array of CarlaPluginDiscoveryHandle.
   */
  GPtrArray * handles;

  /** Array of booleans. */
  GArray * handles_done;

  PluginManager * owner;
} ZCarlaDiscovery;

ZCarlaDiscovery *
z_carla_discovery_new (PluginManager * owner);

void
z_carla_discovery_free (ZCarlaDiscovery * self);

void
z_carla_discovery_start (
  ZCarlaDiscovery * self,
  BinaryType        btype,
  ZPluginProtocol   protocol);

/**
 * @return Whether done.
 */
bool
z_carla_discovery_idle (ZCarlaDiscovery * self);

/**
 * Runs carla discovery for the given arch with the
 * given arguments and returns the output as a
 * newly allocated string.
 */
char *
z_carla_discovery_run (
  PluginArchitecture arch,
  const char *       arg1,
  const char *       arg2);

/**
 * Create a descriptor using carla discovery.
 *
 * @path Path to the plugin bundle.
 * @arch Architecture.
 * @protocol Protocol.
 *
 * @return A newly allocated array of newly
 *   allocated PluginDescriptor's.
 */
PluginDescriptor **
z_carla_discovery_create_descriptors_from_file (
  const char *       path,
  PluginArchitecture arch,
  ZPluginProtocol    protocol);

/**
 * Create a descriptor for the given AU plugin.
 */
PluginDescriptor *
z_carla_discovery_create_au_descriptor_from_info (
  const CarlaCachedPluginInfo * info);

/**
 * Create a descriptor for the given AU plugin.
 */
NONNULL PluginDescriptor *
z_carla_discovery_create_au_descriptor_from_string (
  const char * all_plugins,
  int          idx);

/**
 * Parses plugin info into a new NULL-terminated
 * PluginDescriptor array.
 *
 * @param plugin_path Identifier to use for
 *   debugging.
 *
 * @return A newly allocated array of newly allocated
 *   descriptors, or NULL if no descriptors found.
 */
PluginDescriptor **
z_carla_discovery_parse_plugin_info (const char * plugin_path, char * results);

/**
 * @}
 */

#endif
#endif
