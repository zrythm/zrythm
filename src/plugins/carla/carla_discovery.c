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

#include <stdlib.h>

#include "plugins/plugin_descriptor.h"
#include "plugins/carla/carla_discovery.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/system.h"

#include <CarlaBackend.h>

static ZPluginCategory
get_category_from_carla_category (
  const char * category)
{
#define EQUALS(x) \
  string_is_equal (category, x, false)

  if (EQUALS ("synth"))
    return PC_INSTRUMENT;
  else if (EQUALS ("delay"))
    return PC_DELAY;
  else if (EQUALS ("eq"))
    return PC_EQ;
  else if (EQUALS ("filter"))
    return PC_FILTER;
  else if (EQUALS ("distortion"))
    return PC_DISTORTION;
  else if (EQUALS ("dynamics"))
    return PC_DYNAMICS;
  else if (EQUALS ("modulator"))
    return PC_MODULATOR;
  else if (EQUALS ("utility"))
    return PC_UTILITY;
  else
    return ZPLUGIN_CATEGORY_NONE;

#undef EQUALS
}

/**
 * Create a descriptor for the given plugin path.
 */
PluginDescriptor *
z_carla_discovery_create_vst_descriptor (
  const char * path)
{
  const char * carla_discovery_filename =
    "carla-discovery-native"
#ifdef _WOE32
      ".exe"
#endif
      ;
  char * carla_discovery =
    g_find_program_in_path (
      carla_discovery_filename);

  /* fallback on bindir */
  if (!carla_discovery)
    {
      carla_discovery =
        g_build_filename (
          CONFIGURE_BINDIR, carla_discovery_filename,
          NULL);
      g_message (
        "carla discovery not found locally, falling "
        "back to bindir (%s)",
        carla_discovery);
      g_return_val_if_fail (
        file_exists (carla_discovery), NULL);
    }
  g_return_val_if_fail (
    carla_discovery, NULL);
  char cmd[4000];
  sprintf (
    cmd, "%s vst \"%s\"",
    carla_discovery, path);
  char * results =
    system_get_cmd_output (cmd);
  g_return_val_if_fail (results, NULL);
  g_message (
    "cmd: [[[\n%s\n]]]\n\n"
    "results: [[[\n%s\n]]]", cmd, results);
  char * error =
    string_get_regex_group (
      results,
      "carla-discovery::error::(.*)\\n", 1);
  if (error)
    {
      g_free (error);
      g_warning (
        "error found for %s: %s",
        path, results);
      g_free (results);
      return NULL;
    }
  else if (string_is_equal ("", results, false))
    {
      g_warning (
        "No results returned for %s",
        path);
      g_free (results);
      return NULL;
    }

  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));
  descr->name =
    string_get_regex_group (
      results,
      "carla-discovery::name::(.*)\\n", 1);
  g_return_val_if_fail (descr->name,  NULL);
  descr->author =
    string_get_regex_group (
      results,
      "carla-discovery::maker::(.*)\\n", 1);
  descr->unique_id =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::uniqueId::(.*)\\n", 1, 0);
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
  char * carla_category =
    string_get_regex_group (
      results,
      "carla-discovery::category::(.*)\\n", 1);
  if (carla_category)
    {
      descr->category =
        get_category_from_carla_category (
          carla_category);;
      descr->category_str =
        plugin_descriptor_category_to_string (
          descr->category);
    }
  else
    {
      int hints =
        string_get_regex_group_as_int (
          results,
          "carla-discovery::hints::(.*)\\n",
          1, 0);
      if ((unsigned int) hints & PLUGIN_IS_SYNTH)
        {
          descr->category = PC_INSTRUMENT;
          descr->category_str =
            plugin_descriptor_category_to_string (
              descr->category);
        }
      else
        {
          descr->category = ZPLUGIN_CATEGORY_NONE;
          descr->category_str =
            plugin_descriptor_category_to_string (
              descr->category);
        }
    }

  descr->protocol = PROT_VST;
  descr->arch = ARCH_64;
  descr->path = g_strdup (path);

  g_free (results);

  return descr;
}

#endif
