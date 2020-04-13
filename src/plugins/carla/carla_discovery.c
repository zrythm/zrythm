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

#include "config.h"

#ifdef HAVE_CARLA

#include "plugins/plugin_descriptor.h"
#include "plugins/carla/carla_discovery.h"
#include "utils/string.h"
#include "utils/system.h"

#include <CarlaBackend.h>

static ZPluginCategory
get_category_from_carla_category (
  PluginCategory category)
{
  switch (category)
    {
    case PLUGIN_CATEGORY_SYNTH:
      return PC_INSTRUMENT;
    case PLUGIN_CATEGORY_UTILITY:
      return PC_UTILITY;
    case PLUGIN_CATEGORY_DYNAMICS:
      return PC_DYNAMICS;
    case PLUGIN_CATEGORY_DELAY:
      return PC_DELAY;
    case PLUGIN_CATEGORY_NONE:
    default:
      return ZPLUGIN_CATEGORY_NONE;
    }
}

/**
 * Create a descriptor for the given plugin path.
 */
PluginDescriptor *
z_carla_discovery_create_vst_descriptor (
  const char * path)
{
  char cmd[4000];
  sprintf (
    cmd, "%s vst %s",
    CARLA_DISCOVERY_NATIVE_PATH, path);
  char * results =
    system_get_cmd_output (cmd);
  g_return_val_if_fail (results, NULL);
  char * error =
    string_get_regex_group (
      results,
      "carla-discovery::error::(.*)\\n", 1);
  if (error)
    {
      g_free (error);
      g_message ("error found: %s", results);
      g_free (results);
      return NULL;
    }

  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));
  descr->name =
    string_get_regex_group (
      results,
      "carla-discovery::name::(.*)\\n", 1);
  g_warn_if_fail (descr->name);
  descr->author =
    string_get_regex_group (
      results,
      "carla-discovery::maker::(.*)\\n", 1);
  descr->num_audio_ins =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::audio.ins::(.*)\\n", 1, 0);
  descr->num_audio_outs =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::audio.outs::(.*)\\n", 1, 0);
  descr->num_ctrl_ins =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::parameters.ins::(.*)\\n",
      1, 0);
  descr->num_midi_ins =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::midi.ins::(.*)\\n",
      1, 0);
  descr->num_midi_outs =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::midi.ins::(.*)\\n",
      1, 0);

  /* get category */
  int carla_category =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::category::(.*)\\n", 1, 0);
  descr->category =
    get_category_from_carla_category (
      carla_category);;
  descr->category_str =
    plugin_descriptor_category_to_string (
      descr->category);

  descr->protocol = PROT_VST;
  descr->arch = ARCH_64;
  descr->path = g_strdup (path);

  g_free (results);

  return descr;
}

#endif
