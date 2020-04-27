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
#include "zrythm.h"

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

#ifdef __APPLE__
static ZPluginCategory
carla_category_to_zrythm_category (
  PluginCategory carla_cat)
{
  switch (carla_cat)
    {
    case PLUGIN_CATEGORY_SYNTH:
      return PC_INSTRUMENT;
    case PLUGIN_CATEGORY_DELAY:
      return PC_DELAY;
    case PLUGIN_CATEGORY_EQ:
      return PC_EQ;
    case PLUGIN_CATEGORY_FILTER:
      return PC_FILTER;
    case PLUGIN_CATEGORY_DISTORTION:
      return PC_DISTORTION;
    case PLUGIN_CATEGORY_DYNAMICS:
      return PC_DYNAMICS;
    case PLUGIN_CATEGORY_MODULATOR:
      return PC_MODULATOR;
    case PLUGIN_CATEGORY_UTILITY:
      break;
    case PLUGIN_CATEGORY_OTHER:
    case PLUGIN_CATEGORY_NONE:
    default:
      break;
    }
  return ZPLUGIN_CATEGORY_NONE;
}
#endif

/**
 * Create a descriptor for the given plugin path.
 */
PluginDescriptor *
z_carla_discovery_create_vst_descriptor (
  const char * path,
  bool         thirty_two_bit,
  bool         vst3)
{
  char carla_discovery_filename[60];
  strcpy (
    carla_discovery_filename,
#ifdef _WOE32
    thirty_two_bit ?
      "carla-discovery-win32" :
      "carla-discovery-native"
#else
    "carla-discovery-native"
#endif
    );
  strcat (carla_discovery_filename, BIN_SUFFIX);
  char * carla_discovery =
    g_find_program_in_path (
      carla_discovery_filename);

  /* fallback on bindir */
  if (!carla_discovery)
    {
      char * bindir =
        zrythm_get_bindir ();
      carla_discovery =
        g_build_filename (
          bindir,
          carla_discovery_filename,
          NULL);
      g_free (bindir);
      g_message (
        "carla discovery not found locally, falling "
        "back to bindir (%s)",
        carla_discovery);
      g_return_val_if_fail (
        file_exists (carla_discovery), NULL);
    }
  g_return_val_if_fail (
    carla_discovery, NULL);
  char type[40];
  strcpy (type, vst3 ? "vst3" : "vst");
  char cmd[4000];
  sprintf (
    cmd, "%s %s %s",
    carla_discovery, type, path);
  g_message (
    "cmd: [[[\n%s\n]]]", cmd);
  char * argv[] = {
    carla_discovery, type, (char *) path, NULL };
  char * results =
    system_get_cmd_output (argv, 1200, true);
  g_return_val_if_fail (results, NULL);
  g_message (
    "results: [[[\n%s\n]]]", results);
#ifdef _WOE32
#define LINE_SEP "\\r\\n"
#else
#define LINE_SEP "\\n"
#endif
  char * error =
    string_get_regex_group (
      results,
      "carla-discovery::error::(.*)" LINE_SEP, 1);
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
      "carla-discovery::name::(.*)" LINE_SEP, 1);
  g_return_val_if_fail (descr->name,  NULL);
  descr->author =
    string_get_regex_group (
      results,
      "carla-discovery::maker::(.*)" LINE_SEP, 1);
  descr->unique_id =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::uniqueId::(.*)" LINE_SEP, 1, 0);
  descr->num_audio_ins =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::audio.ins::(.*)" LINE_SEP, 1, 0);
  descr->num_audio_outs =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::audio.outs::(.*)" LINE_SEP, 1, 0);
  descr->num_ctrl_ins =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::parameters.ins::(.*)" LINE_SEP,
      1, 0);
  descr->num_midi_ins =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::midi.ins::(.*)" LINE_SEP,
      1, 0);
  descr->num_midi_outs =
    string_get_regex_group_as_int (
      results,
      "carla-discovery::midi.ins::(.*)" LINE_SEP,
      1, 0);

  /* get category */
  char * carla_category =
    string_get_regex_group (
      results,
      "carla-discovery::category::(.*)" LINE_SEP, 1);
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
          "carla-discovery::hints::(.*)" LINE_SEP,
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

  descr->protocol = vst3 ? PROT_VST3 : PROT_VST;
  descr->arch = thirty_two_bit ? ARCH_32 : ARCH_64;
  descr->path = g_strdup (path);

  /* open all VSTs with carla */
  descr->open_with_carla = true;

  g_free (results);

  return descr;
}

#ifdef __APPLE__
/**
 * Create a descriptor for the given AU plugin.
 */
PluginDescriptor *
z_carla_discovery_create_au_descriptor (
  const CarlaCachedPluginInfo * info)
{
  if (!info || !info->valid)
    return NULL;

  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));
  descr->name = g_strdup (info->name);
  g_return_val_if_fail (descr->name,  NULL);
  descr->author = g_strdup (info->maker);
  descr->num_audio_ins = info->audioIns;
  descr->num_audio_outs = info->audioOuts;
  descr->num_cv_ins = info->cvIns;
  descr->num_cv_outs = info->cvOuts;
  descr->num_ctrl_ins = info->parameterIns;
  descr->num_ctrl_outs = info->parameterOuts;
  descr->num_midi_ins = info->midiIns;
  descr->num_midi_outs = info->midiOuts;

  /* get category */
  if (info->hints & PLUGIN_IS_SYNTH)
    {
      descr->category = PC_INSTRUMENT;
    }
  else
    {
      descr->category =
        carla_category_to_zrythm_category (
          info->category);
    }
  descr->category_str =
    plugin_descriptor_category_to_string (
      descr->category);

  descr->protocol = PROT_AU;
  descr->arch = ARCH_64;
  descr->path = NULL;

  /* open all AUs with carla */
  descr->open_with_carla = true;

  return descr;
}
#endif

#endif
