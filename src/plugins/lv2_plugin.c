/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#define _POSIX_C_SOURCE 200809L /* for mkdtemp */
#define _DARWIN_C_SOURCE        /* for mkdtemp on OSX */

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
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
#include "settings/settings.h"
#include "utils/err_codes.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <lv2/buf-size/buf-size.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/patch/patch.h>
#include <lv2/time/time.h>
#include <lv2/units/units.h>

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
 * Create a port structure from data description.
 *
 * This is called before plugin and Jack
 * instantiation. The remaining instance-specific
 * setup (e.g. buffers) is done later in
 * connect_port().
 *
 * @param port_exists If zrythm port exists (when
 *   loading)
 * @param is_project Whether the plugin is a
 *   project plugin.
 *
 * @return Non-zero if fail.
*/
static int
create_port (
  Lv2Plugin *    lv2_plugin,
  const uint32_t lv2_port_index,
  const float    default_value,
  const int      port_exists,
  bool           is_project)
{
  Lv2Port* const lv2_port =
    &lv2_plugin->ports[lv2_port_index];

  lv2_port->lilv_port =
    lilv_plugin_get_port_by_index (
      lv2_plugin->lilv_plugin, lv2_port_index);
  LilvNode * name_node =
    lilv_port_get_name (
      lv2_plugin->lilv_plugin,
      lv2_port->lilv_port);
  const char * name_str =
    lilv_node_as_string (name_node);
  if (port_exists && is_project)
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
        port_new_with_type (type, flow, name_str);

      port_set_owner_plugin (
        lv2_port->port, lv2_plugin->plugin);
    }
  Port * port = lv2_port->port;
  port->lv2_port = lv2_port;
  lv2_port->evbuf = NULL;
  lv2_port->buf_size = 0;
  lv2_port->index = lv2_port_index;
  port->control = 0.0f;

  const bool optional =
    lilv_port_has_property (
      lv2_plugin->lilv_plugin,
      lv2_port->lilv_port,
      PM_LILV_NODES.core_connectionOptional);

  PortIdentifier * pi =
    &lv2_port->port->id;

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
        isnan (default_value) ?
          0.0f : default_value;
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

      /* set port flags */
      if (lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.pprops_trigger))
        {
          pi->flags |= PORT_FLAG_TRIGGER;
        }
      if (lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_freeWheeling))
        {
          pi->flags |= PORT_FLAG_FREEWHEEL;
        }
      if (lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_reportsLatency))
        {
          pi->flags |= PORT_FLAG_REPORTS_LATENCY;
        }
      if (lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_toggled))
        {
          pi->flags |= PORT_FLAG_TOGGLE;
        }
      else if (lilv_port_has_property (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_integer))
        {
          pi->flags |= PORT_FLAG_INTEGER;
        }

      /* TODO ignore freewheel and other ports that
       * shouldn't be automatable */
      if (pi->flow == FLOW_INPUT)
        {
          pi->flags |= PORT_FLAG_AUTOMATABLE;
        }

      LilvNode * def;
      LilvNode * min;
      LilvNode * max;
      lilv_port_get_range (
        lv2_plugin->lilv_plugin,
        lv2_port->lilv_port, &def, &min, &max);
      if (max)
        {
          port->maxf =
            lilv_node_as_float (max);
          lilv_node_free (max);
        }
      else
        {
          port->maxf = 1.f;
        }
      if (min)
        {
          port->minf =
            lilv_node_as_float (min);
          lilv_node_free (min);
        }
      else
        {
          port->minf = 0.f;
        }
      if (def)
        {
          port->deff =
            lilv_node_as_float (def);
          lilv_node_free (def);
        }
      else
        {
          port->deff = port->minf;
        }
      port->zerof = port->minf;

      /* set unit */
      const LilvNode * port_node =
        lilv_port_get_node (
          lv2_plugin->lilv_plugin,
          lv2_port->lilv_port);
      LilvNodes * units =
        lilv_world_find_nodes (
          LILV_WORLD, port_node,
          LV2_NODES.units_unit, NULL);

#define SET_UNIT(caps,sc) \
  if (lilv_nodes_contains ( \
        units, LV2_NODES.units_##sc)) \
    { \
      port->id.unit = PORT_UNIT_##caps; \
    }

      SET_UNIT (DB, db);
      SET_UNIT (DEGREES, degree);
      SET_UNIT (HZ, hz);
      SET_UNIT (MHZ, mhz);
      SET_UNIT (MS, ms);
      SET_UNIT (SECONDS, s);

      lilv_nodes_free (units);
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
      LilvNode * def;
      LilvNode * min;
      LilvNode * max;
      lilv_port_get_range (
        lv2_plugin->lilv_plugin,
        lv2_port->lilv_port, &def, &min, &max);
      if (max)
        {
          port->maxf =
            lilv_node_as_float (max);
          lilv_node_free (max);
        }
      else
        {
          port->maxf = 1.f;
        }
      if (min)
        {
          port->minf =
            lilv_node_as_float (min);
          lilv_node_free (min);
        }
      else
        {
          port->minf = - 1.f;
        }
      if (def)
        {
          port->deff =
            lilv_node_as_float (def);
          lilv_node_free (def);
        }
      else
        {
          port->deff = port->minf;
        }
      port->zerof = 0.f;
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

      /* set want position flag */
      if (lilv_port_supports_event (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.time_Position))
        {
          pi->flags |= PORT_FLAG_WANT_POSITION;
          lv2_plugin->want_position = 1;
        }
    }
  else if (!optional)
    {
      g_warning (
        "Mandatory lv2_port at %d has unknown "
        "data type",
        lv2_port_index);
      return -1;
    }
  pi->flags |= PORT_FLAG_PLUGIN_CONTROL;

  if (lilv_port_has_property (
        lv2_plugin->lilv_plugin,
        lv2_port->lilv_port,
        PM_LILV_NODES.pprops_notOnGUI))
    {
      pi->flags |= PORT_FLAG_NOT_ON_GUI;
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

  lilv_node_free (min_size);
  lilv_node_free (name_node);

  return 0;
}

void
lv2_plugin_init_loaded (
  Lv2Plugin * self,
  bool        project)
{
  self->magic = LV2_PLUGIN_MAGIC;

  if (!project)
    return;

  Lv2Port * lv2_port;
  for (int i = 0; i < self->num_ports; i++)
    {
      lv2_port = &self->ports[i];
      lv2_port->port =
        port_find_from_identifier (
          &lv2_port->port_id);
      lv2_port->port->lv2_port = lv2_port;
      g_warn_if_fail (
        port_identifier_is_equal (
          &lv2_port->port_id,
          &lv2_port->port->id));
    }
}

/**
 * Create port structures from data (via
 * create_port()) for all ports.
 *
 * @param project Whether this is a project plugin.
 */
static int
lv2_create_or_init_ports (
  Lv2Plugin* self,
  bool       project)
{
  g_return_val_if_fail (
    self->plugin && self->lilv_plugin, -1);

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
          default_values[i], ports_exist,
          project) == 0,
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
          if (port->id.flow == FLOW_INPUT)
            {
              plugin_add_in_port (
                self->plugin, port);
            }
          else if (port->id.flow ==
                   FLOW_OUTPUT)
            {
              plugin_add_out_port (
                self->plugin, port);
            }

          /* remember the identifier for
           * saving/loading */
          port_identifier_copy (
            &lv2_port->port_id,
            &port->id);
        }
    }

  g_warn_if_fail (self->num_ports > 0);

  return 0;
}

/**
 * Allocate port buffers (only necessary for MIDI).
 */
void
lv2_plugin_allocate_port_buffers (
  Lv2Plugin* plugin)
{
  g_return_if_fail (plugin);
  for (int i = 0; i < plugin->num_ports; ++i)
    {
      Lv2Port* const lv2_port =
        &plugin->ports[i];
      g_return_if_fail (lv2_port);
      switch (lv2_port->port_id.type)
        {
        case TYPE_EVENT:
          {
            lv2_evbuf_free (lv2_port->evbuf);
            const size_t buf_size =
              (lv2_port->buf_size > 0) ?
              lv2_port->buf_size :
              AUDIO_ENGINE->midi_buf_size;
            g_return_if_fail (
              buf_size > 0 && plugin->map.map);
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
const void *
lv2_plugin_get_port_value (
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
  if (plugin->plugin->visible)
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
      plugin->plugin->visible &&
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
  if (port->id.flow == FLOW_UNKNOWN ||
      port->id.type == TYPE_UNKNOWN)
    {
      lilv_instance_connect_port(
        lv2_plugin->instance,
        port_index, NULL);
      return;
    }

  /* Connect the port based on its type */
  switch (port->id.type)
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

/**
 * Initializes the plugin features.
 *
 * This is called on instantiation.
 */
static void
set_features (Lv2Plugin * plugin)
{
#define INIT_FEATURE(x,uri) \
  plugin->x.URI = uri; \
  plugin->x.data = NULL
  plugin->ext_data.data_access = NULL;

  INIT_FEATURE (
    map_feature, LV2_URID__map);
  INIT_FEATURE (
    unmap_feature, LV2_URID__unmap);
  INIT_FEATURE (
    make_path_feature_save, LV2_STATE__makePath);
  INIT_FEATURE (
    make_path_feature_temp, LV2_STATE__makePath);
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

#undef INIT_FEATURE

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
    &plugin->make_path_feature_save;
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
  lv2_plugin_process (
    pl, PLAYHEAD->frames, 0);
    /*AUDIO_ENGINE->block_length);*/

  return pl->plugin->latency;
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
  pd->arch = ARCH_64;
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
  if (str)
    {
      pd->category_str =
        string_get_substr_before_suffix (
          str, " Plugin");
    }
  else
    {
      pd->category_str = g_strdup ("Plugin");
    }

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
  Plugin    *  pl,
  const char * uri)
{
  g_message ("Creating from uri: %s...", uri);

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

  g_message ("done");

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
  Plugin * plugin)
{
  Lv2Plugin * self = object_new (Lv2Plugin);

  self->magic = LV2_PLUGIN_MAGIC;

  /* set pointers to each other */
  self->plugin = plugin;
  plugin->lv2 = self;

  return self;
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

bool
lv2_plugin_ui_type_is_external (
  const LilvNode * ui_type)
{
  return
    lilv_node_equals (
      ui_type, PM_LILV_NODES.ui_externalkx) ||
    lilv_node_equals (
      ui_type, PM_LILV_NODES.ui_external);
}

/**
 * Pick the most preferable UI.
 *
 * @param[out] ui (Output) UI of the specific
 *   plugin.
 * @param[out] ui_type UI type (eg, X11).
 *
 * @return Whether a UI was picked.
 */
bool
lv2_plugin_pick_ui (
  const LilvUIs *     uis,
  Lv2PluginPickUiFlag flag,
  const LilvUI **     out_ui,
  const LilvNode **   out_ui_type)
{
  LILV_FOREACH (uis, u, uis)
    {
      const LilvUI* cur_ui =
        lilv_uis_get (uis, u);
      const LilvNodes * ui_types =
        lilv_ui_get_classes (cur_ui);
      g_message (
        "Checking UI: %s",
        lilv_node_as_uri (
          lilv_ui_get_uri (cur_ui)));
      LILV_FOREACH (nodes, t, ui_types)
        {
          const LilvNode * ui_type =
            lilv_nodes_get (ui_types, t);
          const char * ui_type_uri =
            lilv_node_as_uri (ui_type);
          g_message (
            "Found UI type: %s", ui_type_uri);

          bool acceptable = false;
          switch (flag)
            {
            case LV2_PLUGIN_UI_WRAPPABLE:
              acceptable =
                (bool)
                suil_ui_supported (
                  lilv_node_as_uri (
                    PM_LILV_NODES.ui_Gtk3UI),
                  ui_type_uri);
              if (acceptable)
                {
                  if (out_ui_type)
                    *out_ui_type = ui_type;
                  g_message (
                    "Wrappable UI accepted");
                }
              break;
            case LV2_PLUGIN_UI_EXTERNAL:
              if (string_is_equal (
                    ui_type_uri,
                    KX_EXTERNAL_UI_WIDGET,
                    true))
                {
                  acceptable = true;
                  if (out_ui_type)
                    {
                      *out_ui_type =
                        PM_LILV_NODES.
                          ui_externalkx;
                    }
                  g_message (
                    "External KX UI accepted");
                }
              else if (string_is_equal (
                         ui_type_uri,
                         LV2_UI_PREFIX "external",
                         true))
                {
                  acceptable = true;
                  if (out_ui_type)
                    {
                      *out_ui_type =
                        PM_LILV_NODES.ui_external;
                    }
                  g_message (
                    "External non-KX UI accepted");
                }
              break;
            case LV2_PLUGIN_UI_FOR_BRIDGING:
              if (
#ifdef HAVE_CARLA_BRIDGE_LV2_GTK2
                  lilv_node_equals (
                    ui_type,
                    PM_LILV_NODES.ui_GtkUI) ||
#endif
#ifdef HAVE_CARLA_BRIDGE_LV2_GTK3
                  lilv_node_equals (
                    ui_type,
                    PM_LILV_NODES.ui_Gtk3UI) ||
#endif
#ifdef HAVE_CARLA_BRIDGE_LV2_QT5
                  lilv_node_equals (
                    ui_type,
                    PM_LILV_NODES.ui_Qt5UI) ||
#endif
#ifdef HAVE_CARLA_BRIDGE_LV2_QT4
                  lilv_node_equals (
                    ui_type,
                    PM_LILV_NODES.ui_Qt4UI) ||
#endif
                  /*lilv_node_equals (*/
                    /*ui_type,*/
                    /*PM_LILV_NODES.ui_externalkx) ||*/
                  false
                  )
                {
                  acceptable = true;
                  if (out_ui_type)
                    {
                      *out_ui_type = ui_type;
                    }
                  g_message (
                    "UI %s accepted for "
                    "bridging",
                    lilv_node_as_string (
                      ui_type));
                }
              break;
            }

          if (acceptable)
            {
              if (out_ui)
                {
                  *out_ui = cur_ui;
                }
              return true;
            }
        }
    }

  return false;
}

char *
lv2_plugin_get_abs_state_file_path (
  Lv2Plugin * self,
  bool        is_backup)
{
  char * abs_state_dir =
    plugin_get_abs_state_dir (
      self->plugin, is_backup);
  char * state_file_abs_path =
    g_build_filename (
      abs_state_dir, "state.ttl", NULL);

  g_free (abs_state_dir);

  return state_file_abs_path;
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
 * @param use_state_file Whether to use the plugin's
 *   state file to instantiate the plugin.
 * @param preset_uri URI of preset to load.
 * @param state State to load, if loading from
 *   a state. This is used when cloning plugins
 *   for example. The state of the original plugin
 *   is passed here.
 *
 * @return 0 if OK, non-zero if error.
 */
int
lv2_plugin_instantiate (
  Lv2Plugin *  self,
  bool         project,
  bool         use_state_file,
  char *       preset_uri,
  LilvState *  state)
{
  g_message (
    "Instantiating... uri: %s, project: %d, "
    "preset_uri: %s, "
    "state: %p",
    self->plugin->descr->uri, project,
    preset_uri, state);

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

  self->make_path_save.handle = self;
  self->make_path_save.path =
    lv2_state_make_path_save;
  self->make_path_feature_save.data =
    &self->make_path_save;

  self->make_path_temp.handle = self;
  self->make_path_temp.path =
    lv2_state_make_path_temp;
  self->make_path_feature_temp.data =
    &self->make_path_temp;

  self->sched.handle = &self->worker;
  self->sched.schedule_work = lv2_worker_schedule;
  self->sched_feature.data = &self->sched;

  self->ssched.handle = &self->state_worker;
  self->ssched.schedule_work = lv2_worker_schedule;
  self->state_sched_feature.data = &self->ssched;

  self->llog.handle = self;
  lv2_log_set_printf_funcs (&self->llog);
  self->log_feature.data = &self->llog;

  zix_sem_init (&self->exit_sem, 0);
  self->done = &self->exit_sem;

  zix_sem_init(&self->worker.sem, 0);

  /* Load preset, if specified */
  if (!state)
    {
      if (preset_uri)
        {
          LilvNode* preset =
            lilv_new_uri (
              LILV_WORLD, preset_uri);

          lv2_state_load_presets (
            self, NULL, NULL);
          state =
            lilv_state_new_from_world (
              LILV_WORLD, &self->map, preset);
          self->preset = state;
          lilv_node_free(preset);
          if (!state)
            {
              g_warning (
                "Failed to find preset <%s>\n",
                preset_uri);
              return -1;
            }
      }
      else if (use_state_file)
        {
          char * state_file_path =
            lv2_plugin_get_abs_state_file_path (
              self, PROJECT->loading_from_backup);
          state =
            lilv_state_new_from_file (
              LILV_WORLD, &self->map, NULL,
              state_file_path);
          if (!state)
            {
              g_critical (
                "Failed to load state from %s",
                state_file_path);
              return
                ERR_FAILED_TO_LOAD_STATE_FROM_FILE;
            }
          g_free (state_file_path);

          LilvNode * lv2_uri =
            lilv_node_duplicate (
              lilv_state_get_plugin_uri (
                state));

          if (!lv2_uri)
            {
              g_warning (
                "Missing plugin URI, try lv2ls"
                " to list plugins");
            }

          /* Find plugin */
          g_message (
            "Plugin: %s",
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
    }

  /* Set default values for all ports */
  g_return_val_if_fail (
    lv2_create_or_init_ports (
      self, project) == 0, -1);

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

  if (!state)
    {
      /* Not restoring state, load the plugin as a
       * preset to get default */
      state =
        lilv_state_new_from_world (
          LILV_WORLD, &self->map,
          lilv_plugin_get_uri (self->lilv_plugin));
    }

  /* Get appropriate UI */
  self->uis =
    lilv_plugin_get_uis (self->lilv_plugin);
  if (ZRYTHM_TESTING ||
      !g_settings_get_boolean (
        S_P_PLUGINS_UIS, "generic"))
    {
      /* get a wrappable UI */
      g_message ("Looking for wrappable UI...");
      bool ui_picked =
        lv2_plugin_pick_ui (
          self->uis, LV2_PLUGIN_UI_WRAPPABLE,
          &self->ui, &self->ui_type);

      /* if wrappable UI not found, get an external
       * UI */
      if (!ui_picked)
        {
          g_message (
            "No wrappable UI found. "
            "Looking for external UI...");
          ui_picked =
            lv2_plugin_pick_ui (
              self->uis, LV2_PLUGIN_UI_EXTERNAL,
              &self->ui, &self->ui_type);
          if (ui_picked)
            {
              self->has_external_ui = true;
            }
        }
    }

  if (self->ui)
    {
      g_message ("Selected UI: %s (type: %s)",
        lilv_node_as_uri (
          lilv_ui_get_uri(self->ui)),
        lilv_node_as_uri (
          self->ui_type));
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

  self->options_feature.URI = LV2_OPTIONS__options;
  self->options_feature.data =
    (void *) self->options;

  /* Create Plugin <=> UI communication
   * buffers */
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
        PM_LILV_NODES.work_interface) &&
      project)
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
  if (state)
    {
      lv2_state_apply_state (self, state);
    }

  /* Connect ports to buffers */
  for (int i = 0; i < self->num_ports; ++i)
    {
      connect_port (
        self, (uint32_t) i);
    }

  /* Print initial control values */
  if (DEBUGGING)
    {
      for (int i = 0;
           i < self->controls.n_controls; ++i)
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
    }

  g_message ("done");

  return 0;
}

int
lv2_plugin_activate (
  Lv2Plugin * self,
  bool        activate)
{
  /* Activate plugin */
  g_message (
    "%s lilv instance...",
    activate ? "Activating" :"Deactivating");
  if (activate && !self->plugin->activated)
    {
      lilv_instance_activate (self->instance);
    }
  else if (!activate && self->plugin->activated)
    {
      lilv_instance_deactivate (self->instance);
    }

  return 0;
}

int
lv2_plugin_cleanup (
  Lv2Plugin * self)
{
  if (self->plugin->instantiated)
    {
      object_free_w_func_and_null (
        lilv_instance_free, self->instance);
    }

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
  Lv2Plugin * self,
  const long  g_start_frames,
  const nframes_t   nframes)
{
  g_return_if_fail (
    self->plugin->instantiated &&
    self->plugin->activated);

#ifdef HAVE_JACK
  jack_client_t * client = AUDIO_ENGINE->client;
#endif
  int i, p;

  /* If transport state is not as expected, then
   * something has changed */
  const bool xport_changed =
    self->rolling !=
      (TRANSPORT_IS_ROLLING) ||
    self->gframes !=
      g_start_frames ||
    !math_floats_equal (
      self->bpm,
      tempo_track_get_current_bpm (P_TEMPO_TRACK));
# if 0
  if (xport_changed)
    {
      g_message (
        "xport changed lv2_plugin_rolling %d, "
        "gframes vs g start frames %ld %ld, "
        "bpm %f %f",
        self->rolling,
        self->gframes, g_start_frames,
        (double) self->bpm,
        (double) TRANSPORT->bpm);
    }
#endif

  /* let the plugin know if transport state
   * changed */
  uint8_t   pos_buf[256];
  LV2_Atom * lv2_pos = (LV2_Atom*) pos_buf;
  if (xport_changed && self->want_position)
    {
      /* Build an LV2 position object to report
       * change to plugin */
      Position start_pos;
      position_from_frames (
        &start_pos, g_start_frames);
      lv2_atom_forge_set_buffer (
        &self->forge,
        pos_buf,
        sizeof(pos_buf));
      LV2_Atom_Forge * forge = &self->forge;
      LV2_Atom_Forge_Frame frame;
      lv2_atom_forge_object (
        forge, &frame, 0,
        PM_URIDS.time_Position);
      lv2_atom_forge_key (
        forge, PM_URIDS.time_frame);
      lv2_atom_forge_long (
        forge, g_start_frames);
      lv2_atom_forge_key (
        forge, PM_URIDS.time_speed);
      lv2_atom_forge_float (
        forge,
        TRANSPORT->play_state == PLAYSTATE_ROLLING ?
          1.0 : 0.0);
      lv2_atom_forge_key (
        forge, PM_URIDS.time_barBeat);
      lv2_atom_forge_float (
        forge,
        ((float) start_pos.beats - 1) +
        ((float) start_pos.ticks /
          (float) TRANSPORT->ticks_per_beat));
      lv2_atom_forge_key (
        forge, PM_URIDS.time_bar);
      lv2_atom_forge_long (
        forge, start_pos.bars - 1);
      lv2_atom_forge_key (
        forge, PM_URIDS.time_beatUnit);
      lv2_atom_forge_int (
        forge, TRANSPORT->beat_unit);
      lv2_atom_forge_key (
        forge, PM_URIDS.time_beatsPerBar);
      lv2_atom_forge_float (
        forge, (float) TRANSPORT->beats_per_bar);
      lv2_atom_forge_key (
        forge, PM_URIDS.time_beatsPerMinute);
      lv2_atom_forge_float (
        forge,
        tempo_track_get_current_bpm (
          P_TEMPO_TRACK));
    }

  /* Update transport state to expected values for
   * next cycle */
  if (TRANSPORT_IS_ROLLING)
    {
      Position gpos;
      position_from_frames (&gpos, self->gframes);
      transport_position_add_frames (
        TRANSPORT, &gpos, nframes);
      self->gframes = gpos.frames;
      self->rolling = 1;
    }
  else
    {
      self->gframes = g_start_frames;
      self->rolling = 0;
    }
  self->bpm =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);

  /* Prepare port buffers */
  for (p = 0; p < self->num_ports; ++p)
    {
      Lv2Port * lv2_port = &self->ports[p];
      Port * port = lv2_port->port;
      PortIdentifier * id = &port->id;
      if (id->type == TYPE_AUDIO)
        {
          /* connect lv2 ports to plugin port
           * buffers */
          lilv_instance_connect_port (
            self->instance,
            (uint32_t) p, port->buf);
        }
      else if (id->type == TYPE_CV)
        {
          /* connect plugin port directly to a
           * CV buffer in the port. according to
           * the docs it has the same size as an
           * audio port. */
          lilv_instance_connect_port (
            self->instance,
            (uint32_t) p, port->buf);
        }
      else if (id->type == TYPE_EVENT &&
               id->flow == FLOW_INPUT)
        {
          lv2_evbuf_reset(lv2_port->evbuf, true);

          /* Write transport change event if
           * applicable */
          LV2_Evbuf_Iterator iter =
            lv2_evbuf_begin (lv2_port->evbuf);
          if (xport_changed &&
              id->flags & PORT_FLAG_WANT_POSITION)
            {
              lv2_evbuf_write (
                &iter, 0, 0,
                lv2_pos->type, lv2_pos->size,
                (const uint8_t*)
                  LV2_ATOM_BODY (lv2_pos));
            }

          if (self->request_update)
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
      else if (id->type == TYPE_CONTROL &&
               id->flow == FLOW_INPUT)
        {
          /* let the plugin know if freewheeling */
          if (id->flags & PORT_FLAG_FREEWHEEL)
            {
              if (AUDIO_ENGINE->exporting)
                {
                  port->control = port->maxf;
                }
              else
                {
                  port->control = port->minf;
                }
            }
        }
    }
  self->request_update = false;

  /* Run plugin for this cycle */
  const bool send_ui_updates =
    run (self, nframes) &&
    !AUDIO_ENGINE->exporting &&
    self->plugin->ui_instantiated;

  /* Deliver MIDI output and UI events */
  Port * port;
  for (p = 0; p < self->num_ports;
       ++p)
    {
      Lv2Port* const lv2_port =
        &self->ports[p];
      port = lv2_port->port;
      PortIdentifier * pi = &port->id;
      switch (pi->type)
        {
        case TYPE_CONTROL:
          if (pi->flow == FLOW_OUTPUT)
            {
              /* if latency changed, recalc graph */
              if (pi->flags &
                    PORT_FLAG_REPORTS_LATENCY &&
                  self->plugin->latency !=
                    (nframes_t)
                    lv2_port->port->control)
                {
                  g_message (
                    "%s: latency changed from %d "
                    "to %f",
                    pi->label,
                    self->plugin->latency,
                    (double)
                    lv2_port->port->control);
                  EVENTS_PUSH (
                    ET_PLUGIN_LATENCY_CHANGED,
                    NULL);
                  self->plugin->latency =
                    (nframes_t)
                    lv2_port->port->control;
#ifdef HAVE_JACK
                  if (AUDIO_ENGINE->
                        audio_backend ==
                      AUDIO_BACKEND_JACK)
                    {
                      jack_recompute_total_latencies (
                        client);
                    }
#endif
                }

              /* if UI is instantiated */
              if (send_ui_updates &&
                  self->plugin->visible &&
                  !lv2_port->received_ui_event)
                {
                  /* forward event to UI */
                  lv2_ui_send_control_val_event_from_plugin_to_ui (
                    self, lv2_port);
                }
              /* NEWWW END */
            }
          if (send_ui_updates)
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
          break;
        case TYPE_EVENT:
          if (pi->flow == FLOW_OUTPUT)
            {
              for (LV2_Evbuf_Iterator iter =
                     lv2_evbuf_begin (
                       lv2_port->evbuf);
                   lv2_evbuf_is_valid(iter);
                   iter = lv2_evbuf_next (iter))
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
                      if (size != 3)
                        {
                          g_warning (
                            "unhandled event from "
                            "port %s of size %"
                            PRIu32,
                            pi->label, size);
                        }
                      else
                        {
                          /* Write MIDI event to port */
                          midi_events_add_event_from_buf (
                            lv2_port->port->
                              midi_events,
                            frames, body,
                            (int) size, 0);
                        }
                    }

                  /* if UI is instantiated */
                  if (self->plugin->visible &&
                      !lv2_port->old_api)
                    {
                      /* forward event to UI */
                      lv2_ui_send_event_from_plugin_to_ui (
                        self, (uint32_t) p,
                        type, size, body);
                    }
                }

              /* Clear event output for plugin to
               * write to next cycle */
              lv2_evbuf_reset (
                lv2_port->evbuf, false);
            }
        default:
          break;
        }
    }
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
        &port->port_id,
        &port->port->id);
    }
}

/**
 * Populates the banks in the plugin instance.
 */
void
lv2_plugin_populate_banks (
  Lv2Plugin * self)
{
  /* add default bank and preset */
  PluginBank * pl_def_bank =
    plugin_add_bank_if_not_exists (
      self->plugin,
      lilv_node_as_string (
        PM_LILV_NODES.zrythm_default_bank),
      _("Default bank"));
  PluginPreset * pl_def_preset =
    calloc (1, sizeof (PluginPreset));
  pl_def_preset->uri =
    g_strdup (
      lilv_node_as_string (
        PM_LILV_NODES.zrythm_default_preset));
  pl_def_preset->name = g_strdup (_("Init"));
  plugin_add_preset_to_bank (
    self->plugin, pl_def_bank, pl_def_preset);

  LilvNodes * presets =
    lilv_plugin_get_related (
      self->lilv_plugin,
      PM_LILV_NODES.pset_Preset);
  LILV_FOREACH (nodes, i, presets)
    {
      const LilvNode* preset =
        lilv_nodes_get (presets, i);

      LilvNodes * banks =
        lilv_world_find_nodes (
          LILV_WORLD, preset,
          PM_LILV_NODES.pset_bank, NULL);
      PluginBank * pl_bank = NULL;
      if (banks)
        {
          const LilvNode * bank =
            lilv_nodes_get_first (banks);
          const LilvNode * bank_label =
            lilv_world_get (
              LILV_WORLD, bank,
              PM_LILV_NODES.rdfs_label, NULL);
          pl_bank =
            plugin_add_bank_if_not_exists (
              self->plugin,
              lilv_node_as_string (bank),
              bank_label ?
                lilv_node_as_string (bank_label) :
                _("Unnamed bank"));
          lilv_nodes_free (banks);
        }
      else
        {
          pl_bank = pl_def_bank;
        }

      LilvNodes * labels =
        lilv_world_find_nodes (
          LILV_WORLD, preset,
          PM_LILV_NODES.rdfs_label, NULL);
      if (labels)
        {
          const LilvNode* label =
            lilv_nodes_get_first(labels);
          PluginPreset * pl_preset =
            calloc (1, sizeof (PluginPreset));
          pl_preset->uri =
            g_strdup (lilv_node_as_string (preset));
          pl_preset->name =
            g_strdup (lilv_node_as_string (label));
          plugin_add_preset_to_bank (
            self->plugin, pl_bank, pl_preset);
          lilv_nodes_free (labels);

          g_message (
            "found preset %s (<%s>)",
            pl_preset->name, pl_preset->uri);
        }
      else
        {
          g_message (
            "Skipping preset <%s> because it has "
            "no rdfs:label",
            lilv_node_as_string (preset));
        }
    }
  lilv_nodes_free (presets);
}
