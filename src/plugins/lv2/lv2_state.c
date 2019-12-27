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

#include <gtk/gtk.h>

#ifdef HAVE_LV2_STATE
#    include "lv2/lv2plug.in/ns/ext/state/lv2_state.h"
#endif

#define NS_ZRYTHM "https://lv2.zrythm.org/ns/zrythm#"
#define NS_RDF  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS "http://www.w3.org/2000/01/rdf-schema#"
#define NS_XSD  "http://www.w3.org/2001/XMLSchema#"

char *
lv2_state_make_path (
  LV2_State_Make_Path_Handle handle,
  const char*                path)
{
  Lv2Plugin* plugin = (Lv2Plugin*)handle;

  // Create in save directory if saving, otherwise use temp directory
  return
    g_strjoin (
      NULL,
      plugin->save_dir ?
        plugin->save_dir :
        plugin->temp_dir,
      path, NULL);
}

static const void*
get_port_value (
  const char* port_symbol,
  void*       user_data,
  uint32_t*   size,
  uint32_t*   type)
{
  Lv2Plugin*        plugin = (Lv2Plugin*)user_data;
  Lv2Port* port =
    lv2_port_get_by_symbol (
      plugin, port_symbol);
  if (port &&
      port->port->identifier.flow == FLOW_INPUT &&
      port->port->identifier.type == TYPE_CONTROL)
    {
      *size = sizeof(float);
      *type = plugin->forge.Float;
      g_return_val_if_fail (port->port, NULL);
      return &port->port->control;
    }
  *size = *type = 0;
  return NULL;
}

void
lv2_state_save (
  Lv2Plugin*  plugin,
  const char* dir)
{
  plugin->save_dir =
    g_strjoin (NULL, dir, "/", NULL);

  LilvState* const state =
    lilv_state_new_from_instance (
      plugin->lilv_plugin, plugin->instance,
      &plugin->map,
      plugin->temp_dir, dir, dir, dir,
      get_port_value, plugin,
      LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE,
      NULL);

  lilv_state_save (
    LILV_WORLD, &plugin->map, &plugin->unmap,
    state, NULL, dir, "state.ttl");

  lilv_state_free (state);

  g_free (plugin->save_dir);
  plugin->save_dir = NULL;
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

static void
set_port_value (
  const char* port_symbol,
  void*       user_data,
  const void* value,
  uint32_t    size,
  uint32_t    type)
{
  Lv2Plugin*        plugin = (Lv2Plugin*)user_data;
  Lv2Port* port =
    lv2_port_get_by_symbol (
      plugin, port_symbol);
  if (!port) {
          fprintf(stderr, "error: Preset port `%s' is missing\n", port_symbol);
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
        "Preset `%s' value has bad type <%s>\n",
        port_symbol,
        plugin->unmap.unmap (
          plugin->unmap.handle, type));
      return;
    }

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

  if (plugin->window)
    {
      // Update UI
      char buf[sizeof(Lv2ControlChange) + sizeof(fvalue)];
      Lv2ControlChange* ev = (Lv2ControlChange*)buf;
      ev->index    = port->index;
      ev->protocol = 0;
      ev->size     = sizeof(fvalue);
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
  bool must_pause = !plugin->safe_restore && TRANSPORT->play_state == PLAYSTATE_ROLLING;
  if (state)
    {
      if (must_pause)
        {
          g_message ("must pause");
          /*TRANSPORT->play_state = PLAYSTATE_PAUSE_REQUESTED;*/
          /*zix_sem_wait(&TRANSPORT->paused);*/
        }

      g_message ("applying state...");
      lilv_state_restore (state,
                          plugin->instance,
                          set_port_value, plugin,
                          0,
                          plugin->state_features);

      if (must_pause)
        {
          plugin->request_update = true;
          TRANSPORT->play_state     = PLAYSTATE_ROLLING;
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
  LilvState* const state = lilv_state_new_from_instance(
          plugin->lilv_plugin, plugin->instance, &plugin->map,
          plugin->temp_dir, dir, dir, dir,
          get_port_value, plugin,
          LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, NULL);

  if (label)
    {
      lilv_state_set_label(state, label);
    }

  int ret = lilv_state_save(
          LILV_WORLD, &plugin->map, &plugin->unmap, state, uri, dir, filename);

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
