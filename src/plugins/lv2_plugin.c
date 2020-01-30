/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#define _POSIX_C_SOURCE 200809L /* for mkdtemp */
#define _DARWIN_C_SOURCE        /* for mkdtemp on OSX */

#include <assert.h>
#include <math.h>
#include <signal.h>
#ifndef __cplusplus
#    include <stdbool.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2/lv2_log.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2/lv2_worker.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/string.h"

#include <gtk/gtk.h>

#include <lv2/buf-size/buf-size.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/patch/patch.h>
#include <lv2/time/time.h>

#include <lilv/lilv.h>
#include <sratom/sratom.h>
#include <suil/suil.h>

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"
#define NS "http://lv2plug.in/ns/"

#define KX_EXTERNAL_UI_WIDGET "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget"

/**
 * Size factor for UI ring buffers.  The ring size
 * is a few times the size of an event output to
 * give the UI a chance to keep up.  Experiments
 * with Ingen, which can highly saturate its event
 * output, led me to this value.  It really ought
 * to be enough for anybody(TM).
*/
#define N_BUFFER_CYCLES 16

static const bool show_hidden = 1;

/**
 * Returns whether Zrythm supports the given
 * feature.
 */
static int
feature_is_supported (
  Lv2Plugin * plugin,
  const char* uri)
{
  if (string_is_equal (uri, LV2_CORE__isLive, 1))
    return 1;

  for (const LV2_Feature * const * f =
         plugin->features; *f; ++f)
    {
      if (string_is_equal (uri, (*f)->URI, 1))
        return 1;
    }
  return 0;
}

/**
 * Initializes the given feature.
 */
static void
init_feature (
  LV2_Feature* const dest,
  const char* const URI,
  void* data)
{
  dest->URI = URI;
  dest->data = data;
}

/**
 * Create a port structure from data description.
 *
 * This is called before plugin and Jack
 * instantiation. The remaining instance-specific
 * setup (e.g. buffers) is done later in
 * connect_port().
 *
 * @param port_exists If zrythm port exists (when
 *   loading)
 *
 * @return Non-zero if fail.
*/
static int
create_port (
  Lv2Plugin *    lv2_plugin,
  const uint32_t lv2_port_index,
  const float    default_value,
  const int      port_exists)
{
  Lv2Port* const lv2_port =
    &lv2_plugin->ports[lv2_port_index];

  lv2_port->lilv_port =
    lilv_plugin_get_port_by_index (
      lv2_plugin->lilv_plugin, lv2_port_index);
  const LilvNode* sym =
    lilv_port_get_symbol (
      lv2_plugin->lilv_plugin,
      lv2_port->lilv_port);
  if (port_exists)
    {
      lv2_port->port =
        port_find_from_identifier (
          &lv2_port->port_id);
      lv2_port->port->buf =
        realloc (
          lv2_port->port->buf,
          (size_t) AUDIO_ENGINE->block_length *
            sizeof (float));
    }
  else
    {
      char * port_name =
        g_strdup (lilv_node_as_string (sym));
      PortType type = 0;
      PortFlow flow = 0;

      /* Set the lv2_port flow (input or output) */
      if (lilv_port_is_a (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_InputPort))
        {
          flow = FLOW_INPUT;
        }
      else if (lilv_port_is_a (
                 lv2_plugin->lilv_plugin,
                 lv2_port->lilv_port,
                 PM_LILV_NODES.core_OutputPort))
        {
          flow = FLOW_OUTPUT;
        }

      /* Set type */
      if (lilv_port_is_a (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_ControlPort))
        {
          type = TYPE_CONTROL;
        }
      else if (lilv_port_is_a (
                 lv2_plugin->lilv_plugin,
                 lv2_port->lilv_port,
                 PM_LILV_NODES.core_AudioPort))
        {
          type = TYPE_AUDIO;
        }
      else if (lilv_port_is_a (
                 lv2_plugin->lilv_plugin,
                 lv2_port->lilv_port,
                 PM_LILV_NODES.core_CVPort))
        {
          type = TYPE_CV;
        }
      else if (lilv_port_is_a (
                lv2_plugin->lilv_plugin,
                lv2_port->lilv_port,
                PM_LILV_NODES.ev_EventPort))
        {
          type = TYPE_EVENT;
        }
      else if (lilv_port_is_a (
                lv2_plugin->lilv_plugin,
                lv2_port->lilv_port,
                PM_LILV_NODES.atom_AtomPort))
        {
          type = TYPE_EVENT;
        }

      lv2_port->port =
        port_new_with_type (type, flow, port_name);
      g_free (port_name);

      Port * port = lv2_port->port;

      /* the plugin might not have a track assigned
       * to it (eg when cloning) */
      if (lv2_plugin->plugin->track)
        {
          port_set_owner_plugin (
            port, lv2_plugin->plugin);
        }
      else
        {
          Plugin * pl = lv2_plugin->plugin;
          port->identifier.track_pos =
            pl->track_pos;
          port->identifier.plugin_slot =
            pl->slot;
          port->identifier.owner_type =
            PORT_OWNER_TYPE_PLUGIN;
        }
    }
  lv2_port->port->lv2_port = lv2_port;
  lv2_port->evbuf = NULL;
  lv2_port->buf_size = 0;
  lv2_port->index = lv2_port_index;
  lv2_port->port->control = 0.0f;

  const bool optional =
    lilv_port_has_property (
      lv2_plugin->lilv_plugin,
      lv2_port->lilv_port,
      PM_LILV_NODES.core_connectionOptional);

  PortIdentifier * pi =
    &lv2_port->port->identifier;

  /* Set the lv2_port flow (input or output) */
  if (lilv_port_is_a (
        lv2_plugin->lilv_plugin,
        lv2_port->lilv_port,
        PM_LILV_NODES.core_InputPort))
    {
      pi->flow = FLOW_INPUT;
    }
  else if (lilv_port_is_a (
             lv2_plugin->lilv_plugin,
             lv2_port->lilv_port,
             PM_LILV_NODES.core_OutputPort))
    {
      pi->flow = FLOW_OUTPUT;
      lv2_port->port_id.flow = FLOW_OUTPUT;
    }
  else if (!optional)
    {
      g_warning (
        "Mandatory lv2_port at %d has unknown type"
        " (neither input nor output)",
        lv2_port_index);
      return -1;
    }

  /* Set control values */
  if (lilv_port_is_a (
        lv2_plugin->lilv_plugin,
        lv2_port->lilv_port,
        PM_LILV_NODES.core_ControlPort))
    {
      pi->type = TYPE_CONTROL;
      lv2_port->port->control =
        isnan(default_value) ?
        0.0f :
        default_value;
      if (show_hidden ||
          !lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.pprops_notOnGUI))
        {
          Lv2Control * port_control =
            lv2_new_port_control (
              lv2_plugin,
              lv2_port->index);
          lv2_add_control (
            &lv2_plugin->controls,
            port_control);
          lv2_port->lv2_control =
            port_control;
        }
    }
  else if (lilv_port_is_a (
             lv2_plugin->lilv_plugin,
             lv2_port->lilv_port,
             PM_LILV_NODES.core_AudioPort))
    {
      pi->type = TYPE_AUDIO;
    }
  else if (lilv_port_is_a (
             lv2_plugin->lilv_plugin,
             lv2_port->lilv_port,
             PM_LILV_NODES.core_CVPort))
    {
      pi->type = TYPE_CV;
    }
  else if (lilv_port_is_a (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.ev_EventPort))
    {
      pi->type = TYPE_EVENT;
      if (!lv2_port->port->midi_events)
        {
          lv2_port->port->midi_events =
            midi_events_new (lv2_port->port);
        }
      lv2_port->old_api = true;
    }
  else if (lilv_port_is_a (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.atom_AtomPort))
    {
      pi->type = TYPE_EVENT;
      if (!lv2_port->port->midi_events)
        {
          lv2_port->port->midi_events =
            midi_events_new (lv2_port->port);
        }
      lv2_port->old_api = false;
    }
  else if (!optional)
    {
      g_warning ("Mandatory lv2_port at %d has unknown data type",
               lv2_port_index);
      return -1;
    }

  LilvNode* min_size = lilv_port_get(
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.rsz_minimumSize);
  if (min_size && lilv_node_is_int(min_size))
    {
      lv2_port->buf_size =
        (size_t) lilv_node_as_int(min_size);
    }
  lilv_node_free(min_size);

  return 0;
}

void
lv2_plugin_init_loaded (Lv2Plugin * lv2_plgn)
{
  Lv2Port * lv2_port;
  for (int i = 0; i < lv2_plgn->num_ports; i++)
    {
      lv2_port = &lv2_plgn->ports[i];
      lv2_port->port =
        port_find_from_identifier (
          &lv2_port->port_id);
      lv2_port->port->lv2_port = lv2_port;
      g_warn_if_fail (
        port_identifier_is_equal (
          &lv2_port->port_id,
          &lv2_port->port->identifier));
    }
}

/**
 * Create port structures from data (via
 * create_port()) for all ports.
 */
static int
lv2_create_or_init_ports (
  Lv2Plugin* self)
{
  /* zrythm ports exist when loading a
   * project since they are serialized */
  int ports_exist =
    self->num_ports > 0;

  if (ports_exist)
    {
      g_warn_if_fail (
        self->num_ports ==
        (int)
        lilv_plugin_get_num_ports (
          self->lilv_plugin));
    }
  else
    {
      self->num_ports =
        (int)
        lilv_plugin_get_num_ports (
          self->lilv_plugin);
      self->ports =
        (Lv2Port*)
        calloc (
          (size_t) self->num_ports,
          sizeof (Lv2Port));
    }

  float* default_values =
    (float*)
    calloc (
      lilv_plugin_get_num_ports (
        self->lilv_plugin),
      sizeof(float));
  lilv_plugin_get_port_ranges_float (
    self->lilv_plugin,
    NULL, NULL, default_values);

  int i;
  for (i = 0; i < self->num_ports; ++i)
    {
      g_return_val_if_fail (
        create_port (
          self, (uint32_t) i,
          default_values[i], ports_exist) == 0,
        -1);
    }

  /* set control input */
  const LilvPort* control_input =
    lilv_plugin_get_port_by_designation (
      self->lilv_plugin,
      PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_control);
  if (control_input)
    {
      self->control_in =
        (int)
        lilv_port_get_index (
          self->lilv_plugin,
          control_input);
    }

  free (default_values);

  if (!ports_exist)
    {
      /* Set the zrythm plugin ports */
      for (i = 0; i < self->num_ports; ++i)
        {
          Lv2Port * lv2_port = &self->ports[i];
          Port * port = lv2_port->port;
          port->tmp_plugin = self->plugin;
          if (port->identifier.flow == FLOW_INPUT)
            {
              plugin_add_in_port (
                self->plugin, port);
            }
          else if (port->identifier.flow ==
                   FLOW_OUTPUT)
            {
              plugin_add_out_port (
                self->plugin, port);
            }

          /* remember the identifier for
           * saving/loading */
          port_identifier_copy (
            &port->identifier,
            &lv2_port->port_id);
        }
    }

  return 0;
}

/**
 * Allocate port buffers (only necessary for MIDI).
 */
void
lv2_plugin_allocate_port_buffers (
  Lv2Plugin* plugin)
{
  for (int i = 0; i < plugin->num_ports; ++i)
    {
      Lv2Port* const lv2_port =
        &plugin->ports[i];
      Port* port     = lv2_port->port;
      switch (port->identifier.type)
        {
        case TYPE_EVENT:
          {
            lv2_evbuf_free (lv2_port->evbuf);
            const size_t buf_size =
              (lv2_port->buf_size > 0) ?
              lv2_port->buf_size :
              AUDIO_ENGINE->midi_buf_size;
            lv2_port->evbuf =
              lv2_evbuf_new (
                (uint32_t) buf_size,
                plugin->map.map (
                  plugin->map.handle,
                  lilv_node_as_string (
                    PM_LILV_NODES.atom_Chunk)),
                  plugin->map.map (
                    plugin->map.handle,
                  lilv_node_as_string (
                    PM_LILV_NODES.atom_Sequence)));
            lilv_instance_connect_port (
              plugin->instance,
              (uint32_t) i,
              lv2_evbuf_get_buffer (
                lv2_port->evbuf));
          }
        default:
        break;
        }
    }
}

/**
 * Function to get a port value.
 *
 * Used when saving the state.
 * This function MUST set size and type
 * appropriately.
 */
static const void *
lv2_get_port_value (
  const char * port_sym,
  void       * user_data,
  uint32_t   * size,
  uint32_t   * type)
{
  Lv2Plugin * lv2_plugin = (Lv2Plugin *) user_data;

  Lv2Port * port =
    lv2_port_get_by_symbol (
      lv2_plugin, port_sym);
  *size = 0;
  *type = 0;

  if (port)
    {
      *size = sizeof (float);
      *type = PM_URIDS.atom_Float;
      g_return_val_if_fail (port->port, NULL);
      return (const void *) &port->port->control;
    }

  return NULL;
}

Lv2Control*
lv2_plugin_get_control_by_symbol (
  Lv2Plugin * plugin,
  const char* sym)
{
  for (int i = 0;
       i < plugin->controls.n_controls; ++i)
    {
      if (!strcmp (
            lilv_node_as_string (
              plugin->controls.controls[i]->
                symbol),
            sym))
        {
          return plugin->controls.controls[i];
        }
  }
  return NULL;
}

static void
create_controls (
  Lv2Plugin* lv2_plugin,
  bool writable)
{
  const LilvPlugin* plugin =
    lv2_plugin->lilv_plugin;
  LilvWorld* world = LILV_WORLD;
  LilvNode* patch_writable =
    PM_LILV_NODES.patch_writable;
  LilvNode* patch_readable =
    PM_LILV_NODES.patch_readable;

  LilvNodes* properties =
    lilv_world_find_nodes (
      world,
      lilv_plugin_get_uri (plugin),
      writable ? patch_writable : patch_readable,
      NULL);
  LILV_FOREACH (nodes, p, properties)
    {
      const LilvNode* property =
        lilv_nodes_get (properties, p);
      Lv2Control* record   = NULL;

      if (!writable &&
          lilv_world_ask (
            world,
            lilv_plugin_get_uri (plugin),
            patch_writable,
            property))
        {
          // Find existing writable control
          for (int i = 0;
               i < lv2_plugin->controls.n_controls;
               ++i)
            {
              if (lilv_node_equals (
                    lv2_plugin->
                      controls.controls[i]->node,
                    property))
                {
                  record =
                    lv2_plugin->controls.
                      controls[i];
                  record->is_readable = true;
                  break;
                }
            }

          if (record)
            continue;
        }

      record =
        lv2_new_property_control (
          lv2_plugin, property);
      if (writable)
        record->is_writable = true;
      else
        record->is_readable = true;

      if (record->value_type)
        lv2_add_control (
          &lv2_plugin->controls, record);
      else
        {
          g_warning (
            "Parameter <%s> has unknown value "
            "type, ignored",
            lilv_node_as_string(record->node));
          free(record);
        }
    }
  lilv_nodes_free(properties);
}

/**
 * Runs the plugin for this cycle.
 */
static int
run (
  Lv2Plugin* plugin,
  const nframes_t nframes)
{
  /* Read and apply control change events from UI */
  if (plugin->window)
    lv2_ui_read_and_apply_events (plugin, nframes);

  /* Run plugin for this cycle */
  lilv_instance_run (plugin->instance, nframes);

  /* Process any worker replies. */
  lv2_worker_emit_responses (
    &plugin->state_worker, plugin->instance);
  lv2_worker_emit_responses (
    &plugin->worker, plugin->instance);

  /* Notify the plugin the run() cycle is
   * finished */
  if (plugin->worker.iface &&
      plugin->worker.iface->end_run)
    {
      plugin->worker.iface->end_run (
        plugin->instance->lv2_handle);
    }

  /* Check if it's time to send updates to the UI */
  plugin->event_delta_t += nframes;
  bool send_ui_updates = false;
  uint32_t update_frames =
    (uint32_t)
    ((float) AUDIO_ENGINE->sample_rate /
     plugin->plugin->ui_update_hz);
  if (lv2_plugin_has_custom_ui (plugin) &&
      plugin->window &&
      (plugin->event_delta_t > update_frames))
    {
      send_ui_updates = true;
      plugin->event_delta_t = 0;
    }

  return send_ui_updates;
}

/**
 * Connect the port to its buffer.
 */
static void
connect_port(
  Lv2Plugin * lv2_plugin,
  uint32_t port_index)
{
  Lv2Port* const lv2_port =
    &lv2_plugin->ports[port_index];
  Port * port = lv2_port->port;

  /* Connect unsupported ports to NULL (known to
   * be optional by this point) */
  if (port->identifier.flow == FLOW_UNKNOWN ||
      port->identifier.type == TYPE_UNKNOWN)
    {
      lilv_instance_connect_port(
        lv2_plugin->instance,
        port_index, NULL);
      return;
    }

  /* Connect the port based on its type */
  switch (port->identifier.type)
    {
    case TYPE_CONTROL:
      g_return_if_fail (lv2_port->port);
      lilv_instance_connect_port (
        lv2_plugin->instance,
        port_index, &lv2_port->port->control);
      break;
    case TYPE_AUDIO:
      /* connect lv2 ports to plugin port
       * buffers */
      lilv_instance_connect_port (
        lv2_plugin->instance,
        port_index, port->buf);
      break;
    case TYPE_CV:
      /* connect lv2 ports to plugin port
       * buffers */
      lilv_instance_connect_port (
        lv2_plugin->instance,
        port_index, port->buf);
      break;
    case TYPE_EVENT:
      /* already connected to port */
      break;
    default:
      break;
    }
}

#define INIT_FEATURE(x,uri) \
  init_feature (&plugin->x, uri, NULL)

static void
set_features (Lv2Plugin * plugin)
{
  plugin->ext_data.data_access = NULL;

  INIT_FEATURE (
    map_feature, LV2_URID__map);
  INIT_FEATURE (
    unmap_feature, LV2_URID__unmap);
  INIT_FEATURE (
    make_path_feature, LV2_STATE__makePath);
  INIT_FEATURE (
    sched_feature, LV2_WORKER__schedule);
  INIT_FEATURE (
    state_sched_feature, LV2_WORKER__schedule);
  INIT_FEATURE (
    safe_restore_feature,
    LV2_STATE__threadSafeRestore);
  INIT_FEATURE (
    log_feature, LV2_LOG__log);
  INIT_FEATURE (
    options_feature, LV2_OPTIONS__options);
  INIT_FEATURE (
    def_state_feature, LV2_STATE__loadDefaultState);

  /** These features have no data */
  plugin->buf_size_features[0].URI =
    LV2_BUF_SIZE__powerOf2BlockLength;
  plugin->buf_size_features[0].data = NULL;
  plugin->buf_size_features[1].URI =
    LV2_BUF_SIZE__fixedBlockLength;
  plugin->buf_size_features[1].data = NULL;;
  plugin->buf_size_features[2].URI =
    LV2_BUF_SIZE__boundedBlockLength;
  plugin->buf_size_features[2].data = NULL;;

  plugin->features[0] = &plugin->map_feature;
  plugin->features[1] = &plugin->unmap_feature;
  plugin->features[2] = &plugin->sched_feature;
  plugin->features[3] = &plugin->log_feature;
  plugin->features[4] = &plugin->options_feature;
  plugin->features[5] = &plugin->def_state_feature;
  plugin->features[6] =
    &plugin->safe_restore_feature;
  plugin->features[7] =
    &plugin->buf_size_features[0];
  plugin->features[8] =
    &plugin->buf_size_features[1];
  plugin->features[9] =
    &plugin->buf_size_features[2];
  plugin->features[10] = NULL;

  plugin->state_features[0] =
    &plugin->map_feature;
  plugin->state_features[1] =
    &plugin->unmap_feature;
  plugin->state_features[2] =
    &plugin->make_path_feature;
  plugin->state_features[3] =
    &plugin->state_sched_feature;
  plugin->state_features[4] =
    &plugin->safe_restore_feature;
  plugin->state_features[5] =
    &plugin->log_feature;
  plugin->state_features[6] =
    &plugin->options_feature;
  plugin->state_features[7] = NULL;
}

/**
 * Returns the plugin's latency in samples.
 *
 * Searches for a port marked as reportsLatency
 * and gets it value when a 0-size buffer is
 * passed.
 */
nframes_t
lv2_plugin_get_latency (
  Lv2Plugin * pl)
{
  lv2_plugin_process (pl, PLAYHEAD->frames, 2);

  return pl->plugin_latency;
}

/**
 * Returns a newly allocated plugin descriptor for
 * the given LilvPlugin
 * if it can be hosted, otherwise NULL.
 */
PluginDescriptor *
lv2_plugin_create_descriptor_from_lilv (
  const LilvPlugin * lp)
{
  const LilvNode*  lv2_uri =
    lilv_plugin_get_uri (lp);
  const char * uri_str =
    lilv_node_as_string (lv2_uri);
  if (!string_is_ascii (uri_str))
    {
      g_warning ("Invalid plugin URI (not ascii),"
                 " skipping");
      return NULL;
    }

  LilvNode* name =
    lilv_plugin_get_name(lp);

  /* check if we can host the Plugin */
  if (!lv2_uri)
    {
      g_warning ("Plugin URI not found, try lv2ls "
               "to get a list of all plugins");
      lilv_node_free (name);
      return NULL;
    }
  if (!name ||
      !lilv_plugin_get_port_by_index(lp, 0))
    {
      g_warning (
        "Ignoring invalid LV2 plugin %s",
        lilv_node_as_string (
          lilv_plugin_get_uri(lp)));
      lilv_node_free(name);
      return NULL;
    }

  /* check if shared lib exists */
  const LilvNode * lib_uri =
    lilv_plugin_get_library_uri (lp);
  char * path =
    lilv_file_uri_parse (
      lilv_node_as_string (lib_uri),
      NULL);
  if (access (path, F_OK) == -1)
    {
      g_warning ("%s not found, skipping %s",
                 path, lilv_node_as_string (name));
      lilv_node_free(name);
      return NULL;
    }
  lilv_free (path);

  if (lilv_plugin_has_feature (
        lp, PM_LILV_NODES.core_inPlaceBroken))
    {
      g_warning ("Ignoring LV2 plugin \"%s\" "
                 "since it cannot do inplace "
                 "processing.",
                  lilv_node_as_string(name));
      lilv_node_free(name);
      return NULL;
    }

  LilvNodes *required_features =
    lilv_plugin_get_required_features (lp);
  if (lilv_nodes_contains (
        required_features,
        PM_LILV_NODES.bufz_powerOf2BlockLength) ||
      lilv_nodes_contains (
        required_features,
        PM_LILV_NODES.bufz_fixedBlockLength)
     )
    {
      g_message (
        "Ignoring LV2 Plugin %s because "
        "its buffer-size requirements "
        "cannot be satisfied.",
        lilv_node_as_string(name));
      lilv_nodes_free (required_features);
      lilv_node_free (name);
      return NULL;
  }
  lilv_nodes_free(required_features);

  /* set descriptor info */
  PluginDescriptor * pd =
    calloc (1, sizeof (PluginDescriptor));
  pd->protocol = PROT_LV2;
  const char * str = lilv_node_as_string (name);
  pd->name = g_strdup (str);
  lilv_node_free (name);
  LilvNode * author =
    lilv_plugin_get_author_name (lp);
  str = lilv_node_as_string (author);
  pd->author = g_strdup (str);
  lilv_node_free (author);
  const LilvPluginClass* pclass =
    lilv_plugin_get_class(lp);
  const LilvNode * label =
    lilv_plugin_class_get_label(pclass);
  str = lilv_node_as_string (label);
  pd->category_str = g_strdup (str);

  pd->category =
    plugin_descriptor_string_to_category (
      pd->category_str);

  /* count atom-event-ports that feature
   * atom:supports <http://lv2plug.in/ns/ext/midi#MidiEvent>
   */
  int count_midi_out = 0;
  int count_midi_in = 0;
  for (uint32_t i = 0;
       i < lilv_plugin_get_num_ports (lp);
       ++i)
    {
      const LilvPort* port  =
        lilv_plugin_get_port_by_index (lp, i);
      if (lilv_port_is_a (
            lp, port, PM_LILV_NODES.atom_AtomPort))
        {
          LilvNodes* buffer_types =
            lilv_port_get_value (
              lp, port,
              PM_LILV_NODES.atom_bufferType);
          LilvNodes* atom_supports =
            lilv_port_get_value (
              lp, port,
              PM_LILV_NODES.atom_supports);

          if (lilv_nodes_contains (
                buffer_types,
                PM_LILV_NODES.atom_Sequence) &&
              lilv_nodes_contains (
                atom_supports,
                PM_LILV_NODES.midi_MidiEvent))
            {
              if (lilv_port_is_a (
                    lp, port,
                    PM_LILV_NODES.core_InputPort))
                {
                  count_midi_in++;
                }
              if (lilv_port_is_a (
                    lp, port,
                    PM_LILV_NODES.core_OutputPort))
                {
                  count_midi_out++;
                }
            }
          lilv_nodes_free(buffer_types);
          lilv_nodes_free(atom_supports);
        }
    }

  pd->num_audio_ins =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_AudioPort, NULL);
  pd->num_midi_ins =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.ev_EventPort, NULL)
    + count_midi_in;
  pd->num_audio_outs =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.core_AudioPort, NULL);
  pd->num_midi_outs =
    (int)
    lilv_plugin_get_num_ports_of_class(
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.ev_EventPort, NULL)
    + count_midi_out;
  pd->num_ctrl_ins =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_ControlPort,
      NULL);
  pd->num_ctrl_outs =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.core_ControlPort,
      NULL);
  pd->num_cv_ins =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_CVPort,
      NULL);
  pd->num_cv_outs =
    (int)
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.core_CVPort,
      NULL);

  pd->uri = g_strdup (uri_str);

  return pd;
}

/**
 * Creates an LV2 plugin from given uri.
 *
 * Note that this does not instantiate the plugin.
 * For instantiating the plugin using a preset or
 * state file, see lv2_plugin_instantiate.
 *
 * @param pl A newly created Plugin with its
 *   descriptor filled in.
 * @param uri The URI.
 */
Lv2Plugin *
lv2_plugin_new_from_uri (
  Plugin    * pl,
  const char * uri)
{
  LilvNode * lv2_uri =
    lilv_new_uri (LILV_WORLD, uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (
      PM_LILV_NODES.lilv_plugins,
      lv2_uri);
  lilv_node_free (lv2_uri);

  if (!lilv_plugin)
    {
      g_error (
        "Failed to get LV2 Plugin from URI %s",
        uri);
      return NULL;
    }
  Lv2Plugin * lv2_plugin = lv2_plugin_new (pl);

  lv2_plugin->lilv_plugin = lilv_plugin;

  return lv2_plugin;
}

/**
 * Creates a new LV2 plugin using the given Plugin
 * instance.
 *
 * The given plugin instance must be a newly
 * allocated one.
 *
 * @param plugin A newly allocated Plugin instance.
 */
Lv2Plugin *
lv2_plugin_new (
  Plugin *plugin)
{
  Lv2Plugin * lv2_plugin =
    (Lv2Plugin *) calloc (1, sizeof (Lv2Plugin));

  /* set pointers to each other */
  lv2_plugin->plugin = plugin;
  plugin->lv2 = lv2_plugin;

  return lv2_plugin;
}

/**
 * Frees the Lv2Plugin and all its components.
 */
void
lv2_plugin_free (
  Lv2Plugin * lv2_plugin)
{
  /* Wait for finish signal from UI or signal
   * handler */
  zix_sem_wait(&lv2_plugin->exit_sem);
  lv2_plugin->exit = true;

  /* Terminate the worker */
  lv2_worker_finish(&lv2_plugin->worker);

  /* Deactivate audio */
  for (int i = 0; i < lv2_plugin->num_ports; ++i)
    {
      if (lv2_plugin->ports[i].evbuf)
        lv2_evbuf_free (lv2_plugin->ports[i].evbuf);
  }

  /* Deactivate lv2_plugin->*/
  suil_instance_free(lv2_plugin->ui_instance);
  lilv_instance_deactivate(lv2_plugin->instance);
  lilv_instance_free(lv2_plugin->instance);

  /* Clean up */
  free(lv2_plugin->ports);
  zix_ring_free(lv2_plugin->ui_to_plugin_events);
  zix_ring_free(lv2_plugin->plugin_to_ui_events);
  for (LilvNode** n = (LilvNode**)&PM_LILV_NODES;
       *n; ++n)
    {
      lilv_node_free(*n);
    }
  suil_host_free (lv2_plugin->ui_host);
  sratom_free (lv2_plugin->sratom);
  sratom_free (lv2_plugin->ui_sratom);
  lilv_uis_free (lv2_plugin->uis);

  zix_sem_destroy (&lv2_plugin->exit_sem);

  remove (lv2_plugin->temp_dir);
  free (lv2_plugin->temp_dir);
  free (lv2_plugin->ui_event_buf);

  free (lv2_plugin);
}

/**
 * Instantiate the plugin.
 *
 * All of the actual initialization is done here.
 * If this is a new plugin, preset_uri should be
 * empty. If the project is being loaded, preset
 * uri should be the state file path.
 *
 * @param self Plugin to instantiate.
 * @param preset_uri URI of preset to load.
 *
 * @return 0 if OK, non-zero if error.
 */
int
lv2_plugin_instantiate (
  Lv2Plugin *  self,
  char * preset_uri)
{
  int i;

  set_features (self);

  zix_sem_init (&self->work_lock, 1);

  self->map.handle = self;
  self->map.map = lv2_urid_map_uri;
  self->map_feature.data = &self->map;

  self->worker.plugin = self;
  self->state_worker.plugin = self;

  self->unmap.handle = self;
  self->unmap.unmap = lv2_urid_unmap_uri;
  self->unmap_feature.data = &self->unmap;

  lv2_atom_forge_init (&self->forge, &self->map);

  self->env = serd_env_new (NULL);
  serd_env_set_prefix_from_strings (
    self->env,
    (const uint8_t*) "patch",
    (const uint8_t*) LV2_PATCH_PREFIX);
  serd_env_set_prefix_from_strings (
    self->env, (const uint8_t*) "time",
    (const uint8_t*) LV2_TIME_PREFIX);
  serd_env_set_prefix_from_strings (
    self->env, (const uint8_t*) "xsd",
    (const uint8_t*)NS_XSD);

  self->sratom = sratom_new (&self->map);
  self->ui_sratom = sratom_new (&self->map);
  sratom_set_env (self->sratom, self->env);
  sratom_set_env (self->ui_sratom, self->env);

  GError * err = NULL;
  char * templ =
    g_dir_make_tmp ("lv2_self_XXXXXX", &err);
  if (!templ)
    {
      g_warning (
        "LV2 plugin instantiation failed: %s",
        err->message);
      return -1;
    }
  self->temp_dir =
    g_strjoin (NULL, templ, "/", NULL);
  g_free (templ);

  self->make_path.handle = &self;
  self->make_path.path = lv2_state_make_path;
  self->make_path_feature.data = &self->make_path;

  self->sched.handle = &self->worker;
  self->sched.schedule_work = lv2_worker_schedule;
  self->sched_feature.data = &self->sched;

  self->ssched.handle = &self->state_worker;
  self->ssched.schedule_work = lv2_worker_schedule;
  self->state_sched_feature.data = &self->ssched;

  self->llog.handle = self;
  self->llog.printf = lv2_log_printf;
  self->llog.vprintf = lv2_log_vprintf;
  self->log_feature.data = &self->llog;

  zix_sem_init (&self->exit_sem, 0);
  self->done = &self->exit_sem;

  zix_sem_init(&self->worker.sem, 0);

  /* Load preset, if specified */
  if (preset_uri)
    {
      LilvNode* preset =
        lilv_new_uri (
          LILV_WORLD, preset_uri);

      lv2_state_load_presets (self, NULL, NULL);
      self->state =
        lilv_state_new_from_world (
          LILV_WORLD,
          &self->map,
          preset);
      self->preset = self->state;
      lilv_node_free(preset);
      if (!self->state)
        {
          g_warning (
            "Failed to find preset <%s>\n",
            preset_uri);
          return -1;
        }
  }
  else if (self->state_file)
    {
      char * states_dir =
        project_get_states_dir (
          PROJECT, PROJECT->backup_dir != NULL);
      char * state_file_path =
        g_build_filename (
          states_dir,
          self->state_file,
          NULL);
      g_free (states_dir);
      self->state =
        lilv_state_new_from_file (
          LILV_WORLD,
          &self->map,
          NULL,
          state_file_path);
      if (!self->state)
        {
          g_warning (
            "Failed to load state from %s\n",
            state_file_path);
        }
      g_free (state_file_path);

      LilvNode * lv2_uri =
        lilv_node_duplicate (
          lilv_state_get_plugin_uri (
            self->state));

      if (!lv2_uri)
        {
          g_warning ("Missing plugin URI, try lv2ls"
                     " to list plugins");
        }

      /* Find plugin */
      g_message ("Plugin: %s",
                 lilv_node_as_string (lv2_uri));
      g_return_val_if_fail (
        PM_LILV_NODES.lilv_plugins, -1);
      self->lilv_plugin =
        lilv_plugins_get_by_uri (
          PM_LILV_NODES.lilv_plugins,
          lv2_uri);
      lilv_node_free (lv2_uri);
      if (!self->lilv_plugin)
        {
          g_warning ("Failed to find plugin");
        }
    }

  /* Set default values for all ports */
  g_return_val_if_fail (
    lv2_create_or_init_ports (self) == 0, -1);

  self->control_in = -1;

  /* Check that any required features are supported */
  LilvNodes* req_feats =
    lilv_plugin_get_required_features (
      self->lilv_plugin);
  LILV_FOREACH (nodes, f, req_feats)
    {
      const char* uri =
        lilv_node_as_uri (
          lilv_nodes_get (req_feats, f));
      if (!feature_is_supported (self, uri))
        {
          g_warning (
            "Feature %s is not supported\n", uri);
          lilv_nodes_free (req_feats);
          return -1;
        }
    }
  lilv_nodes_free (req_feats);

  /* Check for thread-safe state restore()
   * method. */
  if (lilv_plugin_has_feature (
        self->lilv_plugin,
        PM_LILV_NODES.state_threadSafeRestore))
    {
      self->safe_restore = true;
    }

  if (!self->state)
    {
      /* Not restoring state, load the plugin as a
       * preset to get default */
      self->state =
        lilv_state_new_from_world (
          LILV_WORLD, &self->map,
          lilv_plugin_get_uri (self->lilv_plugin));
    }

  /* Get a self->UI */
  g_message ("Looking for supported UI...");
  self->uis =
    lilv_plugin_get_uis (self->lilv_plugin);
  if (!g_settings_get_int (
        S_PREFERENCES, "generic-plugin-uis"))
    {
      LILV_FOREACH (uis, u, self->uis)
        {
          const LilvUI* this_ui =
            lilv_uis_get (self->uis, u);
          const LilvNodes* types =
            lilv_ui_get_classes (this_ui);
          LILV_FOREACH (nodes, t, types)
            {
              const char * pt =
                lilv_node_as_uri (
                  lilv_nodes_get (types, t));
              g_message ("Found UI: %s", pt);
            }
          if (lilv_ui_is_supported (
                this_ui, suil_ui_supported,
                PM_LILV_NODES.ui_Gtk3UI,
                &self->ui_type))
            {
              /* TODO: Multiple UI support */
              g_message (
                "UI is supported for wrapping with "
                "suil");
              self->ui = this_ui;
              break;
            }
          else
            {
              g_message (
                "UI unsupported for wrapping with "
                "suil");
            }
        }
    }
  else if (!g_settings_get_int (
              S_PREFERENCES,
              "generic-plugin-uis"))
    {
      self->ui =
        lilv_uis_get (
          self->uis, lilv_uis_begin (self->uis));
    }

  /* no wrappable UI found */
  if (!self->ui)
    {
      g_message (
        "Wrappable UI not found, looking for "
        "external");
      LILV_FOREACH (uis, u, self->uis)
        {
          const LilvUI* ui =
            lilv_uis_get (self->uis, u);
          const LilvNodes* types =
            lilv_ui_get_classes (ui);
          LILV_FOREACH(nodes, t, types)
            {
              const char * pt =
                lilv_node_as_uri (
                  lilv_nodes_get (types, t));
              if (!strcmp (
                    pt, KX_EXTERNAL_UI_WIDGET))
                {
                  g_message ("Found UI: %s", pt);
                  g_message (
                    "External KX UI selected");
                  self->has_external_ui = 1;
                  self->ui = ui;
                  self->ui_type =
                    PM_LILV_NODES.ui_externalkx;
                }
              else if (!strcmp (
                        pt,
                        LV2_UI_PREFIX "external"))
                {
                  g_message ("Found UI: %s", pt);
                  g_message (
                    "External UI selected");
                  self->has_external_ui = 1;
                  self->ui_type =
                    PM_LILV_NODES.ui_external;
                  self->ui = ui;
                }
            }
        }
    }

  /* Create ringbuffers for UI if necessary */
  if (self->ui)
    {
      g_message ("Selected UI: %s",
        lilv_node_as_uri (
          lilv_ui_get_uri(self->ui)));
    }
  else
    {
      g_message ("Selected UI: None");
    }

  /* Create port and control structures */
  create_controls (self, true);
  create_controls (self, false);

  if (self->comm_buffer_size == 0)
    {
      /* The UI ring is fed by self->output
       * ports (usually one), and the UI
       * updates roughly once per cycle.
       * The ring size is a few times the
       *  size of the MIDI output to give the UI
       *  a chance to keep up.  The UI
       * should be able to keep up with 4 cycles,
       * and tests show this works
       * for me, but this value might need
       * increasing to avoid overflows.
      */
      self->comm_buffer_size =
        (uint32_t)
          (AUDIO_ENGINE->midi_buf_size *
          N_BUFFER_CYCLES);
  }

  /* The UI can only go so fast, clamp to reasonable limits */
  self->comm_buffer_size =
    MAX (4096, self->comm_buffer_size);
  g_message ("Comm buffers: %d bytes",
             self->comm_buffer_size);
  g_message ("Update rate:  %.01f Hz",
             (double) self->plugin->ui_update_hz);

  static float samplerate = 0.f;
  static int blocklength = 0;
  static int midi_buf_size = 0;

  samplerate =
    (float) AUDIO_ENGINE->sample_rate;
  blocklength =
    (int) AUDIO_ENGINE->block_length;
  midi_buf_size =
    (int) AUDIO_ENGINE->midi_buf_size;

  /* Build options array to pass to plugin */
  const LV2_Options_Option options[] =
    {
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.param_sampleRate,
        sizeof(float),
        PM_URIDS.atom_Float,
        &samplerate },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.bufsz_minBlockLength,
        sizeof(int32_t),
        PM_URIDS.atom_Int,
        &blocklength },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.bufsz_maxBlockLength,
        sizeof(int32_t),
        PM_URIDS.atom_Int,
        &blocklength },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.bufsz_sequenceSize,
        sizeof(int32_t),
        PM_URIDS.atom_Int,
        &midi_buf_size },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.ui_updateRate,
        sizeof(float),
        PM_URIDS.atom_Float,
        &self->plugin->ui_update_hz },
      { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
    };
  memcpy (
    self->options, options,
    sizeof (self->options));

  init_feature (
    &self->options_feature,
    LV2_OPTIONS__options,
    (void*)self->options);

  /* Create Plugin <=> UI communication buffers */
  self->ui_to_plugin_events =
    zix_ring_new (self->comm_buffer_size);
  self->plugin_to_ui_events =
    zix_ring_new (self->comm_buffer_size);
  zix_ring_mlock (self->ui_to_plugin_events);
  zix_ring_mlock (self->plugin_to_ui_events);

  /* Instantiate the plugin */
  self->instance =
    lilv_plugin_instantiate (
      self->lilv_plugin,
      AUDIO_ENGINE->sample_rate,
      self->features);
  if (!self->instance)
    {
      g_warning ("Failed to instantiate plugin");
      return -1;
    }
  g_message ("Lilv plugin instantiated");

  self->ext_data.data_access =
    lilv_instance_get_descriptor (
      self->instance)->extension_data;

  lv2_plugin_allocate_port_buffers (self);

  /* Create workers if necessary */
  if (lilv_plugin_has_extension_data (
        self->lilv_plugin,
        PM_LILV_NODES.work_interface))
    {
      const LV2_Worker_Interface* iface =
        (const LV2_Worker_Interface*)
        lilv_instance_get_extension_data (
          self->instance, LV2_WORKER__interface);

      lv2_worker_init (
        self, &self->worker, iface, true);
      if (self->safe_restore)
        lv2_worker_init (
          self, &self->state_worker,
          iface, false);
    }

  /* Apply loaded state to plugin instance if
   * necessary */
  if (self->state)
    {
      lv2_state_apply_state (self, self->state);
    }

  /* Connect ports to buffers */
  for (i = 0; i < self->num_ports; ++i)
    {
      connect_port (
        self, (uint32_t) i);
    }

  /* Print initial control values */
  if (DEBUGGING)
    for (i = 0; i < self->controls.n_controls; ++i)
      {
        Lv2Control* control =
          self->controls.controls[i];
        if (control->type == PORT)
          {
            Lv2Port* port =
              &self->ports[control->index];
            g_return_val_if_fail (port->port, -1);
            g_message (
              "%s = %f",
              lv2_port_get_symbol_as_string (
                self, port),
              (double) port->port->control);
          }
      }

  /* Activate plugin */
  g_message ("Activating instance...");
  lilv_instance_activate (self->instance);
  g_message ("Instance activated");

  return 0;
}

/**
 * Processes the plugin for this cycle.
 *
 * @param g_start_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
lv2_plugin_process (
  Lv2Plugin * lv2_plugin,
  const long  g_start_frames,
  const nframes_t   nframes)
{
#ifdef HAVE_JACK
  jack_client_t * client = AUDIO_ENGINE->client;
#endif
  int i, p;

  /* If transport state is not as expected, then
   * something has changed */
  const bool xport_changed =
    lv2_plugin->rolling !=
      (TRANSPORT_IS_ROLLING) ||
    lv2_plugin->gframes !=
      g_start_frames ||
    !math_floats_equal (
      lv2_plugin->bpm,
      TRANSPORT->bpm, 0.001f);

  /* let the plugin know if transport state
   * changed */
  uint8_t   pos_buf[256];
  LV2_Atom * lv2_pos = (LV2_Atom*) pos_buf;
  if (xport_changed)
    {
      /* Build an LV2 position object to report
       * change to plugin */
      Position start_pos;
      position_from_frames (
        &start_pos, g_start_frames);
      lv2_atom_forge_set_buffer (
        &lv2_plugin->forge,
        pos_buf,
        sizeof(pos_buf));
      LV2_Atom_Forge * forge = &lv2_plugin->forge;
      LV2_Atom_Forge_Frame frame;
      lv2_atom_forge_object (
        forge,
        &frame,
        0,
        PM_URIDS.time_Position);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_frame);
      lv2_atom_forge_long (
        forge,
        g_start_frames);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_speed);
      lv2_atom_forge_float (
        forge,
        TRANSPORT->play_state == PLAYSTATE_ROLLING ?
          1.0 : 0.0);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_barBeat);
      lv2_atom_forge_float (
        forge,
        (float) start_pos.beats - 1 +
        ((float) start_pos.ticks /
          (float) TRANSPORT->ticks_per_beat));
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_bar);
      lv2_atom_forge_long (
        forge,
        start_pos.bars - 1);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_beatUnit);
      lv2_atom_forge_int (
        forge,
        TRANSPORT->beat_unit);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_beatsPerBar);
      lv2_atom_forge_float (
        forge,
        (float) TRANSPORT->beats_per_bar);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_beatsPerMinute);
      lv2_atom_forge_float (
        forge, TRANSPORT->bpm);
    }

  /* Update transport state to expected values for
   * next cycle */
  if (TRANSPORT_IS_ROLLING)
    {
      lv2_plugin->gframes =
        transport_frames_add_frames (
          TRANSPORT, lv2_plugin->gframes,
          nframes);
      lv2_plugin->rolling = 1;
    }
  else
    {
      lv2_plugin->rolling = 0;
    }
  lv2_plugin->bpm = TRANSPORT->bpm;

  /* Prepare port buffers */
  for (p = 0; p < lv2_plugin->num_ports; ++p)
    {
      Lv2Port * lv2_port = &lv2_plugin->ports[p];
      Port * port = lv2_port->port;
      if (port->identifier.type == TYPE_AUDIO)
        {
          /* connect lv2 ports to plugin port
           * buffers */
          lilv_instance_connect_port (
            lv2_plugin->instance,
            (uint32_t) p, port->buf);
        }
      else if (port->identifier.type == TYPE_CV)
        {
          /* connect plugin port directly to a
           * CV buffer in the port. according to
           * the docs it has the same size as an
           * audio port. */
          lilv_instance_connect_port (
            lv2_plugin->instance,
            (uint32_t) p, port->buf);
        }
      else if (
        port->identifier.type == TYPE_EVENT &&
        port->identifier.flow == FLOW_INPUT)
        {
          lv2_evbuf_reset(lv2_port->evbuf, true);

          /* Write transport change event if
           * applicable */
          LV2_Evbuf_Iterator iter =
            lv2_evbuf_begin (lv2_port->evbuf);
          if (xport_changed)
            {
              lv2_evbuf_write (
                &iter, 0, 0,
                lv2_pos->type, lv2_pos->size,
                (const uint8_t*)
                  LV2_ATOM_BODY (lv2_pos));
            }

          if (lv2_plugin->request_update)
            {
              /* Plugin state has changed, request
               * an update */
              const LV2_Atom_Object get = {
                { sizeof(LV2_Atom_Object_Body),
                  PM_URIDS.atom_Object },
                { 0, PM_URIDS.patch_Get } };
              lv2_evbuf_write (
                &iter, 0, 0,
                get.atom.type, get.atom.size,
                (const uint8_t*)
                  LV2_ATOM_BODY (&get));
            }

          if (port->midi_events->num_events > 0)
            {
              /* Write MIDI input */
              for (i = 0;
                   i < port->midi_events->num_events;
                   ++i)
                {
                  MidiEvent * ev =
                    &port->midi_events->events[i];
                  g_message (
                    "writing plugin input event %d",
                    i);
                  midi_event_print (ev);
                  lv2_evbuf_write (
                    &iter,
                    ev->time, 0,
                    PM_URIDS.midi_MidiEvent,
                    3, ev->raw_buffer);
                }
            }
          midi_events_clear (
            port->midi_events, 0);
      }
    }
  lv2_plugin->request_update = false;

  /* Run plugin for this cycle */
  const bool send_ui_updates =
    run (lv2_plugin, nframes) &&
    !AUDIO_ENGINE->exporting &&
    lv2_plugin->plugin->ui_instantiated;

  /* Deliver MIDI output and UI events */
  Port * port;
  for (p = 0; p < lv2_plugin->num_ports;
       ++p)
    {
      Lv2Port* const lv2_port =
        &lv2_plugin->ports[p];
      port = lv2_port->port;
      if (port->identifier.flow == FLOW_OUTPUT &&
          port->identifier.type == TYPE_CONTROL &&
          lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_reportsLatency))
        {
          if (!math_floats_equal (
                (float) lv2_plugin->plugin_latency,
                lv2_port->port->control, 0.001f))
            {
              lv2_plugin->plugin_latency =
                (uint32_t) lv2_port->port->control;
#ifdef HAVE_JACK
              jack_recompute_total_latencies (
                client);
#endif
            }
        }
      else if (port->identifier.flow ==
                 FLOW_OUTPUT &&
               port->identifier.type == TYPE_EVENT)
          {
            for (LV2_Evbuf_Iterator iter =
                   lv2_evbuf_begin(lv2_port->evbuf);
                 lv2_evbuf_is_valid(iter);
                 iter = lv2_evbuf_next(iter))
              {
                // Get event from LV2 buffer
                uint32_t frames, subframes,
                         type, size;
                uint8_t* body;
                lv2_evbuf_get (
                  iter, &frames, &subframes,
                  &type, &size, &body);

                /* if midi event */
                if (body && type ==
                    PM_URIDS.
                      midi_MidiEvent)
                  {
                    /* Write MIDI event to port */
                    midi_events_add_event_from_buf (
                      lv2_port->port->midi_events,
                      frames, body, (int) size);
                  }

                /* if UI is instantiated */
                if (lv2_plugin->window &&
                    !lv2_port->old_api)
                  {
                    /* forward event to UI */
                    lv2_ui_send_event_from_plugin_to_ui (
                      lv2_plugin, (uint32_t) p,
                      type, size, body);
                  }
              }

            /* Clear event output for plugin to
             * write to next cycle */
            lv2_evbuf_reset (
              lv2_port->evbuf, false);
          }
        else if (
          send_ui_updates &&
          port->identifier.type == TYPE_CONTROL)
          {
            /* ignore ports that received a UI
             * event at the start of a cycle
             * (otherwise these causes trembling
             * while changing them) */
            if (lv2_port->received_ui_event)
              {
                lv2_port->received_ui_event = 0;
                continue;
              }
          }
    }

}

int
lv2_plugin_save_state_to_file (
  Lv2Plugin * lv2_plugin,
  const char * dir)
{
  LilvState * state = lilv_state_new_from_instance (
    lv2_plugin->lilv_plugin,
    lv2_plugin->instance,
    &lv2_plugin->map,
    NULL,
    dir,
    dir,
    /*dir, [> FIXME use lv2_plugin->save_dir when opening a project <]*/
    dir, /* FIXME use lv2_plugin->save_dir when opening a project */
    lv2_get_port_value,
    (void *) lv2_plugin,
    LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
    lv2_plugin->state_features);

  g_return_val_if_fail (state, -1);

  char * label =
    g_strdup_printf (
      "%s.ttl",
      lv2_plugin->plugin->descr->name);
  /* FIXME check for return value */
  int rc =
    lilv_state_save (
      LILV_WORLD, &lv2_plugin->map,
      &lv2_plugin->unmap, state, NULL,
      dir, label);
  if (rc)
    {
      g_warning ("Lilv save state failed");
      return -1;
    }
  char * tmp = g_path_get_basename (dir);
  lv2_plugin->state_file =
    g_build_filename (tmp, label, NULL);
  g_free (label);
  g_free (tmp);
  lilv_state_free (state);

  return 0;
}

/**
 * Updates theh PortIdentifier's in the Lv2Plugin.
 */
void
lv2_plugin_update_port_identifiers (
  Lv2Plugin * self)
{
  Lv2Port * port;
  for (int i = 0; i < self->num_ports; i++)
    {
      port = &self->ports[i];
      port_identifier_copy (
        &port->port->identifier,
        &port->port_id);
    }
}

/**
 * Saves the current state to a string (returned).
 *
 * MUST be free'd by caller.
 */
int
lv2_plugin_save_state_to_str (
  Lv2Plugin * lv2_plugin)
{
  g_warn_if_reached ();

  /* TODO */
  /*LilvState * state =*/
    /*lilv_state_new_from_instance (*/
      /*lv2_plugin->lilv_plugin,*/
      /*lv2_plugin->instance,*/
      /*&lv2_plugin->map,*/
      /*NULL,*/
      /*NULL,*/
      /*NULL,*/
      /*lv2_get_port_value,*/
      /*(void *) lv2_plugin,*/
      /*LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,*/
      /*lv2_plugin->state_features);*/

  return -1;
}
