/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "audio/transport.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "lilv/lilv.h"
#include <lv2/presets/presets.h>
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"

#define STATE_FILENAME "state.ttl"

typedef enum
{
  Z_PLUGINS_LV2_LV2_STATE_ERROR_INSTANTIATION_FAILED,
  Z_PLUGINS_LV2_LV2_STATE_ERROR_FAILED,
} ZPluginsCarlaNativePluginError;

#define Z_PLUGINS_LV2_LV2_STATE_ERROR \
  z_plugins_lv2_lv2_state_error_quark ()
GQuark
z_plugins_lv2_lv2_state_error_quark (void);
G_DEFINE_QUARK (
  z-plugins-lv2-lv2-state-error-quark, z_plugins_lv2_lv2_state_error)

/* not used - lilv handles these */
#if 0
/**
 * LV2 state mapPath feature - abstract path
 * callback.
 */
char *
lv2_state_get_abstract_path (
  LV2_State_Map_Path_Handle handle,
  const char *              absolute_path)
{
  Lv2Plugin * lv2 = (Lv2Plugin *) handle;
  char * absolute_state_path =
    plugin_get_abs_state_dir (
      lv2->plugin, F_NOT_BACKUP);

  GFile * parent =
    g_file_new_for_path (absolute_state_path);
  GFile * descendant =
    g_file_new_for_path (absolute_path);

  char * abstract_path =
    g_file_get_relative_path (parent, descendant);

  g_object_unref (parent);
  g_object_unref (descendant);
  g_free (absolute_state_path);

  g_message (
    "%s: '%s' -> '%s'",
    __func__, absolute_path, abstract_path);

  g_return_val_if_fail (abstract_path, NULL);

  return abstract_path;
}

/**
 * LV2 state mapPath feature - absolute path
 * callback.
 */
char *
lv2_state_get_absolute_path (
  LV2_State_Map_Path_Handle handle,
  const char *              abstract_path)
{
  Lv2Plugin * lv2 = (Lv2Plugin *) handle;
  char * absolute_state_path =
    plugin_get_abs_state_dir (
      lv2->plugin, F_NOT_BACKUP);

  GFile * parent =
    g_file_new_for_path (absolute_state_path);

  GFile * absolute_path_file =
    g_file_resolve_relative_path (
      parent, abstract_path);
  char * absolute_path =
    g_file_get_path (absolute_path_file);

  g_object_unref (parent);
  g_object_unref (absolute_path_file);
  g_free (absolute_state_path);

  g_message (
    "%s: '%s' -> '%s'",
    __func__, abstract_path, absolute_path);

  g_return_val_if_fail (absolute_path, NULL);

  return absolute_path;
}

/**
 * LV2 State freePath implementation.
 */
void
lv2_state_free_path (
  LV2_State_Free_Path_Handle handle,
  char *                     path)
{
  g_free (path);
}

/**
 * LV2 State makePath feature for save only.
 *
 * Should be passed to LV2_State_Interface::save().
 * and this function must return an absolute path.
 */
char *
lv2_state_make_path_save (
  LV2_State_Make_Path_Handle handle,
  const char*                path)
{
  Lv2Plugin * pl = (Lv2Plugin *) handle;
  g_return_val_if_fail (IS_LV2_PLUGIN (pl), NULL);

  char * full_path =
    plugin_get_abs_state_dir (
      pl->plugin, F_NOT_BACKUP);

  /* make sure the path is absolute */
  g_warn_if_fail (g_path_is_absolute (full_path));

  char * ret =
    g_build_filename (full_path, path, NULL);

  g_free (full_path);

  return ret;
}
#endif

/**
 * LV2 State makePath feature for temporary files.
 *
 * Should be passed to LV2_Descriptor::instantiate()
 * and this function must return an absolute path.
 */
char *
lv2_state_make_path_temp (
  LV2_State_Make_Path_Handle handle,
  const char *               path)
{
  Lv2Plugin * pl = (Lv2Plugin *) handle;
  g_return_val_if_fail (IS_LV2_PLUGIN (pl), NULL);

  /* if plugin already has a state dir, use it,
   * otherwise create it */
  if (!pl->temp_dir)
    {
      GError * err = NULL;
      pl->temp_dir = g_dir_make_tmp ("XXXXXX", &err);
      if (err)
        {
          g_critical (
            "An error occurred making a temp dir");
          return NULL;
        }
    }

  return g_build_filename (pl->temp_dir, path, NULL);
}

/**
 * Saves the plugin state to the filesystem and
 * returns the state.
 */
LilvState *
lv2_state_save_to_file (Lv2Plugin * pl, bool is_backup)
{
  g_return_val_if_fail (
    pl->plugin->instantiated && pl->instance, NULL);

  char * abs_state_dir = plugin_get_abs_state_dir (
    pl->plugin, is_backup);
  char * copy_dir = project_get_path (
    PROJECT, PROJECT_PATH_PLUGIN_EXT_COPIES, false);
  char * link_dir = project_get_path (
    PROJECT, PROJECT_PATH_PLUGIN_EXT_LINKS, false);

  LilvState * const state =
    lilv_state_new_from_instance (
      pl->lilv_plugin, pl->instance, &pl->map,
      pl->temp_dir, copy_dir, link_dir,
      abs_state_dir, lv2_plugin_get_port_value, pl,
      LV2_STATE_IS_PORTABLE, pl->state_features);
  g_return_val_if_fail (state, NULL);

  int rc = lilv_state_save (
    LILV_WORLD, &pl->map, &pl->unmap, state, NULL,
    abs_state_dir, STATE_FILENAME);
  if (rc)
    {
      g_critical ("Lilv save state failed");
      return NULL;
    }

  g_message (
    "Lilv state saved to %s", pl->plugin->state_dir);

  return state;
}

/**
 * Saves the plugin state into a new LilvState that
 * can be applied to any plugin with the same URI
 * (like clones).
 *
 * Must be free'd with lilv_state_free().
 */
LilvState *
lv2_state_save_to_memory (Lv2Plugin * plugin)
{
  LilvState * state = lilv_state_new_from_instance (
    plugin->lilv_plugin, plugin->instance,
    &plugin->map, plugin->temp_dir, NULL, NULL,
    NULL, lv2_plugin_get_port_value, plugin,
    LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
    plugin->state_features);

  g_message (
    "Lilv state saved to memory for plugin %s",
    plugin->plugin->setting->descr->name);

  return state;
}

static void
set_port_value (
  const char * port_symbol,
  void *       user_data,
  const void * value,
  uint32_t     size,
  uint32_t     type)
{
  Lv2Plugin * plugin = (Lv2Plugin *) user_data;
  Plugin *    pl = plugin->plugin;
  char        pl_str[800];
  plugin_print (plugin->plugin, pl_str, 800);

  Port * port =
    plugin_get_port_by_symbol (pl, port_symbol);
  if (!port)
    {
      g_warning (
        "[%s] Preset port %s is missing", pl_str,
        port_symbol);
      return;
    }

  LV2_Atom_Forge forge = plugin->main_forge;

  float fvalue;
  if (type == forge.Float)
    fvalue = *(const float *) value;
  else if (type == forge.Double)
    fvalue = (float) *(const double *) value;
  else if (type == forge.Int)
    fvalue = (float) *(const int32_t *) value;
  else if (type == forge.Long)
    fvalue = (float) *(const int64_t *) value;
  else
    {
      g_warning (
        "[%s] Preset `%s' value has bad type <%s>",
        pl_str, port_symbol,
        plugin->unmap.unmap (
          plugin->unmap.handle, type));
      return;
    }
  g_debug (
    "(lv2 state): setting %s=%f...", port_symbol,
    (double) fvalue);

  if (TRANSPORT->play_state != PLAYSTATE_ROLLING)
    {
      /* Set value on port struct directly */
      port->control = fvalue;
    }
  else
    {
      /* Send value to running plugin */
      lv2_ui_send_event_from_ui_to_plugin (
        plugin, port->lilv_port_index,
        sizeof (fvalue), 0, &fvalue);
    }

  if (pl->visible)
    {
      /* update UI */
      char buf
        [sizeof (Lv2ControlChange) + sizeof (fvalue)];
      Lv2ControlChange * ev =
        (Lv2ControlChange *) buf;
      ev->index = port->lilv_port_index;
      ev->protocol = 0;
      ev->size = sizeof (fvalue);
      *(float *) ev->body = fvalue;
      zix_ring_write (
        plugin->plugin_to_ui_events, buf,
        sizeof (buf));
    }
}

void
lv2_state_apply_state (
  Lv2Plugin * plugin,
  LilvState * state)
{
  char pl_str[800];
  plugin_print (plugin->plugin, pl_str, 800);

  /* if plugin does not support thread safe
   * restore, stop the engine */
  EngineState engine_state;
  bool        engine_paused = false;
  if (!plugin->safe_restore && AUDIO_ENGINE->run)
    {
      g_message (
        "plugin '%s' does not support safe "
        "restore, pausing engine",
        pl_str);
      engine_wait_for_pause (
        AUDIO_ENGINE, &engine_state, Z_F_NO_FORCE);
      g_return_if_fail (!AUDIO_ENGINE->run);
      engine_paused = true;
    }

  g_message (
    "applying state for LV2 plugin '%s'...", pl_str);
  lilv_state_restore (
    state, plugin->instance, set_port_value, plugin,
    0, plugin->state_features);
  g_message (
    "LV2 state applied for plugin '%s'", pl_str);

  if (engine_paused)
    {
      plugin->request_update = true;
      g_message ("resuming engine");
      engine_resume (AUDIO_ENGINE, &engine_state);
      g_return_if_fail (AUDIO_ENGINE->run);
    }
}

/**
 * Applies the given preset, or the preset in the
 * path if @ref preset is NULL.
 *
 * @return Whether the preset was applied.
 */
bool
lv2_state_apply_preset (
  Lv2Plugin *      plugin,
  const LilvNode * preset,
  const char *     path,
  GError **        error)
{
  lilv_state_free (plugin->preset);
  if (preset)
    {
      plugin->preset = lilv_state_new_from_world (
        LILV_WORLD, &plugin->map, preset);
    }
  else
    {
      plugin->preset = lilv_state_new_from_file (
        LILV_WORLD, &plugin->map, NULL, path);
    }

  if (!plugin->preset)
    {
      g_set_error (
        error, Z_PLUGINS_LV2_LV2_STATE_ERROR,
        Z_PLUGINS_LV2_LV2_STATE_ERROR_FAILED, "%s",
        _ ("Failed to apply LV2 preset"));
      return false;
    }

  lv2_state_apply_state (plugin, plugin->preset);
  return true;
}

/**
 * Saves the preset.
 */
int
lv2_state_save_preset (
  Lv2Plugin *  plugin,
  const char * dir,
  const char * uri,
  const char * label,
  const char * filename)
{
  LilvState * const state =
    lilv_state_new_from_instance (
      plugin->lilv_plugin, plugin->instance,
      &plugin->map, plugin->temp_dir, dir, dir, dir,
      lv2_plugin_get_port_value, plugin,
      LV2_STATE_IS_PORTABLE, NULL);

  if (label)
    {
      lilv_state_set_label (state, label);
    }

  int ret = lilv_state_save (
    LILV_WORLD, &plugin->map, &plugin->unmap, state,
    uri, dir, filename);

  lilv_state_free (plugin->preset);
  plugin->preset = state;

  return ret;
}

/**
 * Deletes the current preset.
 */
int
lv2_state_delete_current_preset (Lv2Plugin * plugin)
{
  if (!plugin->preset)
    {
      return 1;
    }

  lilv_world_unload_resource (
    LILV_WORLD, lilv_state_get_uri (plugin->preset));
  lilv_state_delete (LILV_WORLD, plugin->preset);
  lilv_state_free (plugin->preset);
  plugin->preset = NULL;
  return 0;
}

int
lv2_state_load_presets (
  Lv2Plugin * plugin,
  PresetSink  sink,
  void *      data)
{
  LilvNodes * presets = lilv_plugin_get_related (
    plugin->lilv_plugin,
    PM_GET_NODE (LV2_PRESETS__Preset));
  LILV_FOREACH (nodes, i, presets)
    {
      const LilvNode * preset =
        lilv_nodes_get (presets, i);
      lilv_world_load_resource (LILV_WORLD, preset);
      if (!sink)
        {
          continue;
        }

      LilvNodes * labels = lilv_world_find_nodes (
        LILV_WORLD, preset,
        PM_GET_NODE (LILV_NS_RDFS "label"), NULL);
      if (labels)
        {
          const LilvNode * label =
            lilv_nodes_get_first (labels);
          sink (plugin, preset, label, data);
          lilv_nodes_free (labels);
        }
      else
        {
          g_message (
            "Preset <%s> has no rdfs:label\n",
            lilv_node_as_string (preset));
        }
    }
  lilv_nodes_free (presets);

  return 0;
}

int
lv2_state_unload_presets (Lv2Plugin * plugin)
{
  LilvNodes * presets = lilv_plugin_get_related (
    plugin->lilv_plugin,
    PM_GET_NODE (LV2_PRESETS__Preset));
  LILV_FOREACH (nodes, i, presets)
    {
      const LilvNode * preset =
        lilv_nodes_get (presets, i);
      lilv_world_unload_resource (
        LILV_WORLD, preset);
    }
  lilv_nodes_free (presets);

  return 0;
}
