/*
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "lilv/lilv.h"

#include "audio/transport.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#define NS_ZRYTHM "https://lv2.zrythm.org/ns/zrythm#"
#define NS_RDF  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS "http://www.w3.org/2000/01/rdf-schema#"
#define NS_XSD  "http://www.w3.org/2001/XMLSchema#"

#define STATE_FILENAME "state.ttl"

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

/**
 * LV2 State makePath feature for temporary files.
 *
 * Should be passed to LV2_Descriptor::instantiate()
 * and this function must return an absolute path.
 */
char *
lv2_state_make_path_temp (
  LV2_State_Make_Path_Handle handle,
  const char*                path)
{
  Lv2Plugin * pl = (Lv2Plugin *) handle;
  g_return_val_if_fail (IS_LV2_PLUGIN (pl), NULL);

  /* if plugin already has a state dir, use it,
   * otherwise create it */
  if (!pl->temp_dir)
    {
      GError * err = NULL;
      pl->temp_dir =
        g_dir_make_tmp ("XXXXXX", &err);
      if (err)
        {
          g_critical (
            "An error occurred making a temp dir");
          return NULL;
        }
    }

  return
    g_build_filename (pl->temp_dir, path, NULL);
}

/**
 * Saves the plugin state to the filesystem and
 * returns the state.
 */
LilvState *
lv2_state_save_to_file (
  Lv2Plugin *  pl,
  bool         is_backup)
{
  g_return_val_if_fail (
    pl && pl->plugin->instantiated &&
    pl->instance, NULL);

  char * abs_state_dir =
    plugin_get_abs_state_dir (pl->plugin, is_backup);
  char * copy_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_PLUGIN_EXT_COPIES,
      false);
  char * link_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_PLUGIN_EXT_LINKS,
      false);

  LilvState* const state =
    lilv_state_new_from_instance (
      pl->lilv_plugin, pl->instance,
      &pl->map,
      pl->temp_dir, copy_dir, link_dir,
      abs_state_dir,
      lv2_plugin_get_port_value, pl,
      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
      pl->state_features);

  int rc =
    lilv_state_save (
      LILV_WORLD, &pl->map, &pl->unmap,
      state, NULL, abs_state_dir, STATE_FILENAME);
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
lv2_state_save_to_memory (
  Lv2Plugin * plugin)
{
  LilvState * state =
    lilv_state_new_from_instance (
      plugin->lilv_plugin, plugin->instance,
      &plugin->map,
      plugin->temp_dir, NULL, NULL, NULL,
      lv2_plugin_get_port_value, plugin,
      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
      plugin->state_features);

  g_message (
    "Lilv state saved to memory for plugin %s",
    plugin->plugin->descr->name);

  return state;
}

static void
set_port_value (
  const char* port_symbol,
  void*       user_data,
  const void* value,
  uint32_t    size,
  uint32_t    type)
{
  Lv2Plugin * plugin = (Lv2Plugin*)user_data;
  Lv2Port* port =
    lv2_port_get_by_symbol (plugin, port_symbol);
  if (!port)
    {
      g_critical (
        "Preset port %s is missing", port_symbol);
      return;
  }

  float fvalue;
  if (type == plugin->forge.Float)
    fvalue = *(const float*)value;
  else if (type == plugin->forge.Double)
    fvalue = (float) *(const double*)value;
  else if (type == plugin->forge.Int)
    fvalue = (float) *(const int32_t*)value;
  else if (type == plugin->forge.Long)
    fvalue = (float) *(const int64_t*)value;
  else
    {
      g_warning (
        "Preset `%s' value has bad type <%s>",
        port_symbol,
        plugin->unmap.unmap (
          plugin->unmap.handle, type));
      return;
    }
  g_message (
    "(lv2 state): setting %s=%f...",
    port_symbol, (double) fvalue);

  if (TRANSPORT->play_state != PLAYSTATE_ROLLING)
    {
      // Set value on port struct directly
      port->port->control = fvalue;
    } else {
      // Send value to running plugin
      lv2_ui_send_event_from_ui_to_plugin (
        plugin, port->index,
        sizeof(fvalue), 0, &fvalue);
    }

  if (plugin->plugin->visible)
    {
      // Update UI
      char buf[sizeof (Lv2ControlChange) +
        sizeof (fvalue)];
      Lv2ControlChange* ev = (Lv2ControlChange*)buf;
      ev->index = port->index;
      ev->protocol = 0;
      ev->size = sizeof(fvalue);
      *(float*)ev->body = fvalue;
      zix_ring_write (
        plugin->plugin_to_ui_events, buf,
        sizeof(buf));
    }
}

void
lv2_state_apply_state (
  Lv2Plugin* plugin,
  LilvState* state)
{
  bool must_pause =
    !plugin->safe_restore &&
    TRANSPORT->play_state == PLAYSTATE_ROLLING;
  if (state)
    {
      if (must_pause)
        {
          g_message ("must pause");
          TRANSPORT->play_state =
            PLAYSTATE_PAUSE_REQUESTED;
          g_usleep (10000);
          /*zix_sem_wait(&TRANSPORT->paused);*/
        }

      g_message ("applying state...");
      lilv_state_restore (
        state, plugin->instance,
        set_port_value, plugin, 0,
        plugin->state_features);

      if (must_pause)
        {
          plugin->request_update = true;
          TRANSPORT->play_state = PLAYSTATE_ROLLING;
        }
    }
}

int
lv2_state_apply_preset (
  Lv2Plugin* plugin,
  const LilvNode* preset)
{
  lilv_state_free (plugin->preset);
  plugin->preset =
    lilv_state_new_from_world (
      LILV_WORLD, &plugin->map, preset);
  lv2_state_apply_state (plugin, plugin->preset);
  return 0;
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
  LilvState* const state =
    lilv_state_new_from_instance (
      plugin->lilv_plugin, plugin->instance,
      &plugin->map,
      plugin->temp_dir, dir, dir, dir,
      lv2_plugin_get_port_value, plugin,
      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
      NULL);

  if (label)
    {
      lilv_state_set_label (state, label);
    }

  int ret =
    lilv_state_save (
      LILV_WORLD, &plugin->map, &plugin->unmap,
      state, uri, dir, filename);

  lilv_state_free(plugin->preset);
  plugin->preset = state;

  return ret;
}

/**
 * Deletes the current preset.
 */
int
lv2_state_delete_current_preset (
  Lv2Plugin* plugin)
{
  if (!plugin->preset) {
    return 1;
  }

  lilv_world_unload_resource(LILV_WORLD, lilv_state_get_uri(plugin->preset));
  lilv_state_delete(LILV_WORLD, plugin->preset);
  lilv_state_free(plugin->preset);
  plugin->preset = NULL;
  return 0;
}

int
lv2_state_load_presets (
  Lv2Plugin* plugin,
  PresetSink sink,
  void* data)
{
  LilvNodes* presets =
    lilv_plugin_get_related (
      plugin->lilv_plugin,
      PM_LILV_NODES.pset_Preset);
  LILV_FOREACH(nodes, i, presets) {
    const LilvNode* preset = lilv_nodes_get(presets, i);
    lilv_world_load_resource(LILV_WORLD, preset);
    if (!sink) {
      continue;
    }

    LilvNodes* labels =
      lilv_world_find_nodes (
        LILV_WORLD, preset,
        PM_LILV_NODES.rdfs_label, NULL);
    if (labels) {
      const LilvNode* label = lilv_nodes_get_first(labels);
      sink(plugin, preset, label, data);
      lilv_nodes_free(labels);
    } else {
      fprintf(stderr, "Preset <%s> has no rdfs:label\n",
              lilv_node_as_string(lilv_nodes_get(presets, i)));
    }
  }
  lilv_nodes_free(presets);

  return 0;
}

int
lv2_state_unload_presets (
  Lv2Plugin* plugin)
{
  LilvNodes* presets =
    lilv_plugin_get_related (
      plugin->lilv_plugin,
      PM_LILV_NODES.pset_Preset);
  LILV_FOREACH(nodes, i, presets) {
    const LilvNode* preset = lilv_nodes_get(presets, i);
    lilv_world_unload_resource(LILV_WORLD, preset);
  }
  lilv_nodes_free(presets);

  return 0;
}

