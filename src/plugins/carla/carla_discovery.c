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

#include "zrythm-config.h"

#ifdef HAVE_CARLA

#include <stdlib.h>

#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_manager.h"
#include "plugins/carla/carla_discovery.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/system.h"
#include "zrythm.h"

#include <lv2/instance-access/instance-access.h>

#include <CarlaBackend.h>

static ZPluginCategory
get_category_from_carla_category (
  const char * category)
{
#define EQUALS(x) \
  string_is_equal (category, x)

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

/**
 * Returns the absolute path to carla-discovery-*
 * as a newly allocated string.
 */
char *
z_carla_discovery_get_discovery_path (
  PluginArchitecture arch)
{
  char carla_discovery_filename[60];
  strcpy (
    carla_discovery_filename,
#ifdef _WOE32
    arch == ARCH_32 ?
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
        zrythm_get_dir (
          ZRYTHM_DIR_SYSTEM_BINDIR);
      carla_discovery =
        g_build_filename (
          bindir, carla_discovery_filename, NULL);
      g_free (bindir);
      g_message (
        "carla discovery not found locally, falling "
        "back to bindir (%s)",
        carla_discovery);
      g_return_val_if_fail (
        file_exists (carla_discovery), NULL);
    }

  return carla_discovery;
}

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
z_carla_discovery_parse_plugin_info (
  const char * plugin_path,
  char *       results)
{
  PluginDescriptor ** descriptors =
    calloc (1, sizeof (PluginDescriptor *));
  int num_descriptors = 0;

  const char * discovery_init_txt =
    "carla-discovery::init::-----------";
  const char * discovery_end_txt =
    "carla-discovery::end::------------";

#ifdef _WOE32
#define LINE_SEP "\\r\\n"
#else
#define LINE_SEP "\\n"
#endif
  char * error =
    string_get_regex_group (
      results,
      "carla-discovery::error::(.*)" LINE_SEP, 1);
  char * offset_str;
  if (error)
    {
      g_free (error);
      g_message (
        "error found for %s: %s",
        plugin_path, results);
      goto free_descriptors_and_return_null;
    }
  else if (string_is_equal ("", results))
    {
      g_message (
        "No results returned for %s",
        plugin_path);
      goto free_descriptors_and_return_null;
    }

  offset_str =
    strstr (results, discovery_init_txt);
  while (offset_str)
    {
      /* get info for this plugin */
      char * plugin_info =
        string_get_substr_before_suffix (
          offset_str, discovery_end_txt);

      PluginDescriptor * descr =
        calloc (1, sizeof (PluginDescriptor));
      descr->name =
        string_get_regex_group (
          plugin_info,
          "carla-discovery::name::(.*)" LINE_SEP, 1);
      if (!descr->name)
        {
          g_warning (
            "Failed to get plugin name for %s. "
            "skipping...", plugin_path);
          goto free_descriptors_and_return_null;
        }
      descr->author =
        string_get_regex_group (
          plugin_info,
          "carla-discovery::maker::(.*)" LINE_SEP, 1);
      descr->unique_id =
        string_get_regex_group_as_int (
          plugin_info,
          "carla-discovery::uniqueId::(.*)" LINE_SEP, 1, 0);
      descr->num_audio_ins =
        string_get_regex_group_as_int (
          plugin_info,
          "carla-discovery::audio.ins::(.*)" LINE_SEP, 1, 0);
      descr->num_audio_outs =
        string_get_regex_group_as_int (
          plugin_info,
          "carla-discovery::audio.outs::(.*)" LINE_SEP, 1, 0);
      descr->num_ctrl_ins =
        string_get_regex_group_as_int (
          plugin_info,
          "carla-discovery::parameters.ins::(.*)" LINE_SEP,
          1, 0);
      descr->num_midi_ins =
        string_get_regex_group_as_int (
          plugin_info,
          "carla-discovery::midi.ins::(.*)" LINE_SEP,
          1, 0);
      descr->num_midi_outs =
        string_get_regex_group_as_int (
          plugin_info,
          "carla-discovery::midi.ins::(.*)" LINE_SEP,
          1, 0);

      /* get label for AU */
      descr->uri =
        string_get_regex_group (
          plugin_info,
          "carla-discovery::label::(.*)" LINE_SEP, 1);

      /* get category */
      char * carla_category =
        string_get_regex_group (
          plugin_info,
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
              plugin_info,
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

      num_descriptors++;
      descriptors =
        realloc (
          descriptors,
          (size_t) (num_descriptors + 1) *
            sizeof (PluginDescriptor *));
      descriptors[num_descriptors - 1] = descr;

      g_free (plugin_info);

      offset_str =
        strstr (
          &offset_str[1], discovery_init_txt);
    }

  g_message (
    "%d descriptors found for %s",
    num_descriptors, plugin_path);
  if (num_descriptors == 0)
    {
      goto free_descriptors_and_return_null;
    }

  /* NULL-terminate */
  descriptors[num_descriptors] = NULL;

  return descriptors;

free_descriptors_and_return_null:
  for (int i = 0; i < num_descriptors; i++)
    {
      plugin_descriptor_free (descriptors[i]);
    }
  free (descriptors);
  return NULL;
}

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
  PluginProtocol     protocol)
{
  const char * type = NULL;
  switch (protocol)
    {
    case PROT_VST3:
      type = "vst3";
      break;
    case PROT_VST:
      type = "vst";
      break;
    case PROT_DSSI:
      type = "dssi";
      break;
    case PROT_LADSPA:
      type = "ladspa";
      break;
    default:
      break;
    }
  g_return_val_if_fail (type, NULL);

  char * results =
    z_carla_discovery_run (arch, type, path);
  if (!results)
    {
      g_warning (
        "Failed to get results for %s", path);
      return NULL;
    }
  g_message (
    "results: [[[\n%s\n]]]", results);

  PluginDescriptor ** descriptors =
    z_carla_discovery_parse_plugin_info (
      path, results);
  g_free (results);
  if (!descriptors)
    {
      g_debug (
        "No plugin info was parsed from %s", path);
      return NULL;
    }

  PluginDescriptor * descr = NULL;
  int i = 0;
  while ((descr = descriptors[i++]))
    {
      descr->protocol = protocol;
      descr->arch = arch;
      descr->path = g_strdup (path);
      GFile * file =
        g_file_new_for_path (descr->path);
      descr->ghash = g_file_hash (file);
      g_object_unref (file);

      /* open all VSTs with carla */
      descr->open_with_carla = true;
    }

  return descriptors;
}

/**
 * Runs carla discovery for the given arch with the
 * given arguments and returns the output as a
 * newly allocated string.
 */
char *
z_carla_discovery_run (
  PluginArchitecture arch,
  const char *       arg1,
  const char *       arg2)
{
  char * carla_discovery =
    z_carla_discovery_get_discovery_path (arch);
  g_return_val_if_fail (
    carla_discovery, NULL);
  char cmd[4000];
  sprintf (
    cmd, "%s %s %s",
    carla_discovery, arg1, arg2);
  g_message (
    "cmd: [[[\n%s\n]]]", cmd);
  const char * argv[] = {
    carla_discovery, (char *) arg1,
    (char *) arg2, NULL };
#if 0
  char * results =
    system_get_cmd_output (argv, 1200, true);
#endif
  char * res;
  int ret =
    system_run_cmd_w_args (
      argv, 4000, &res, NULL, true);
  if (ret == 0)
    {
      return res;
    }
  else
    {
      return NULL;
    }
}

CarlaBridgeMode
z_carla_discovery_get_bridge_mode (
  const PluginDescriptor * descr)
{
  if (descr->protocol == PROT_LV2)
    {
      /* TODO if the UI and DSP binary is the same
       * file, bridge the whole plugin */
      LilvNode * lv2_uri =
        lilv_new_uri (LILV_WORLD, descr->uri);
      const LilvPlugin * lilv_plugin =
        lilv_plugins_get_by_uri (
          LILV_PLUGINS, lv2_uri);
      lilv_node_free (lv2_uri);
      LilvUIs * uis =
        lilv_plugin_get_uis (lilv_plugin);
      const LilvUI * picked_ui;
      const LilvNode * picked_ui_type;
      bool needs_bridging =
        lv2_plugin_pick_ui (
          uis, LV2_PLUGIN_UI_FOR_BRIDGING,
          &picked_ui, &picked_ui_type);
      if (needs_bridging)
        {
          const LilvNode * ui_uri =
            lilv_ui_get_uri (picked_ui);
          LilvNodes * ui_required_features =
            lilv_world_find_nodes (
              LILV_WORLD, ui_uri,
              PM_GET_NODE (
                LV2_CORE__requiredFeature),
              NULL);
          if (lilv_nodes_contains (
                ui_required_features,
                PM_GET_NODE (LV2_DATA_ACCESS_URI)) ||
              lilv_nodes_contains (
                ui_required_features,
                PM_GET_NODE (
                  LV2_INSTANCE_ACCESS_URI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__Qt4UI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__Qt5UI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__GtkUI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__Gtk3UI)))
            {
              return CARLA_BRIDGE_FULL;
            }
          else
            {
              return CARLA_BRIDGE_UI;
            }
          lilv_nodes_free (ui_required_features);
        }
      else /* does not need bridging */
        {
          return CARLA_BRIDGE_NONE;
        }
      lilv_uis_free (uis);
    }
  else if (descr->arch == ARCH_32)
    {
      return CARLA_BRIDGE_FULL;
    }
  else
    {
      return CARLA_BRIDGE_NONE;
    }

  g_return_val_if_reached (CARLA_BRIDGE_NONE);
}

/**
 * Create a descriptor for the given AU plugin.
 */
PluginDescriptor *
z_carla_discovery_create_au_descriptor_from_info (
  const CarlaCachedPluginInfo * info)
{
  if (!info || !info->valid)
    return NULL;

  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));
  descr->name = g_strdup (info->name);
  g_return_val_if_fail (descr->name,  NULL);
  descr->author = g_strdup (info->maker);
  descr->num_audio_ins = (int) info->audioIns;
  descr->num_audio_outs = (int) info->audioOuts;
  descr->num_cv_ins = (int) info->cvIns;
  descr->num_cv_outs = (int) info->cvOuts;
  descr->num_ctrl_ins = (int) info->parameterIns;
  descr->num_ctrl_outs = (int) info->parameterOuts;
  descr->num_midi_ins = (int) info->midiIns;
  descr->num_midi_outs = (int) info->midiOuts;

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

/**
 * Create a descriptor for the given AU plugin.
 */
PluginDescriptor *
z_carla_discovery_create_au_descriptor_from_string (
  const char * all_plugins,
  int          idx)
{
  const char * discovery_end_txt =
    "carla-discovery::end::------------";

  char * cur_str = g_strdup (all_plugins);

  /* replace cur_str with the following parts */
  for (int i = 0; i < idx; i++)
    {
      char * next_val =
        string_remove_until_after_first_match (
          cur_str, discovery_end_txt);
      g_free (cur_str);
      cur_str = next_val;
    }

  char * plugin_info =
    string_get_substr_before_suffix (
      cur_str, discovery_end_txt);
  g_free (cur_str);

  char id[50];
  sprintf (id, "%d", idx);
  PluginDescriptor ** descriptors =
    z_carla_discovery_parse_plugin_info (
      id, plugin_info);
  g_free (plugin_info);
  if (!descriptors || !descriptors[0])
    return NULL;

  PluginDescriptor * descr = descriptors[0];
  free (descriptors);
  descr->protocol = PROT_AU;
  descr->arch = ARCH_64;

  /* open all AUs with carla */
  descr->open_with_carla = true;

  return descr;
}

#endif
