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
 * Create a descriptor for the given AU plugin.
 */
PluginDescriptor *
z_carla_discovery_create_au_descriptor_from_info (
  const CarlaCachedPluginInfo * info);

/**
 * @}
 */

#endif
#endif
