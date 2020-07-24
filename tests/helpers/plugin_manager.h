/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin manager helper.
 */

#ifndef __TEST_HELPERS_PLUGIN_MANAGER_H__
#define __TEST_HELPERS_PLUGIN_MANAGER_H__

#include "zrythm-test-config.h"

#include "plugins/lv2_plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/objects.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <glib.h>

/**
 * @addtogroup tests
 *
 * @{
 */

PluginDescriptor *
test_plugin_manager_get_plugin_descriptor (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, pl_bundle);
  lilv_world_load_bundle (LILV_WORLD, path);
  lilv_node_free (path);

  plugin_manager_scan_plugins (
    PLUGIN_MANAGER, 1.0, NULL);

  PluginDescriptor * descr = NULL;
  for (int i = 0; i < PLUGIN_MANAGER->num_plugins;
       i++)
    {
      if (string_is_equal (
            PLUGIN_MANAGER->plugin_descriptors[i]->
              uri, pl_uri, true))
        {
          descr =
            plugin_descriptor_clone (
              PLUGIN_MANAGER->plugin_descriptors[i]);
        }
    }

  /* open with carla if requested */
  descr->open_with_carla = with_carla;

  return descr;
}

/**
 * @}
 */

#endif
