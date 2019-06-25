/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

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

/**
 * \file
 *
 * Implementation of LV2 Plugin.
 */


#define _POSIX_C_SOURCE 200809L /* for mkdtemp */
#define _DARWIN_C_SOURCE        /* for mkdtemp on OSX */

#include <assert.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef __cplusplus
#    include <stdbool.h>
#endif

#ifdef _WIN32
#    include <io.h>  /* for _mktemp */
#    define snprintf _snprintf
#else
#    include <unistd.h>
#endif

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2_gtk.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/control.h"
#include "plugins/lv2/log.h"
#include "plugins/lv2/suil.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/worker.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/io.h"
#include "utils/string.h"

#include <gtk/gtk.h>

#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>

#include <lilv/lilv.h>
#include <sratom/sratom.h>


#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"
#define NS_EXT "http://lv2plug.in/ns/ext/"


#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* Size factor for UI ring buffers.  The ring size is a few times the size of
   an event output to give the UI a chance to keep up.  Experiments with Ingen,
   which can highly saturate its event output, led me to this value.  It
   really ought to be enough for anybody(TM).
*/
#define N_BUFFER_CYCLES 16


bool show_hidden = 1;
uint32_t buffer_size = 0;
bool dump = false;
bool print_controls = 0;

/** Return true iff Zrythm supports the given feature. */
static bool
_feature_is_supported (Lv2Plugin * plugin,
                      const char* uri)
{
  if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive"))
    {
      return true;
    }
  for (const LV2_Feature*const* f = plugin->features; *f; ++f)
    {
      if (!strcmp(uri, (*f)->URI))
        {
          return true;
        }
    }
  return false;
}


static void
init_feature(LV2_Feature* const dest, const char* const URI, void* data)
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
 * activate_port().
 *
 * @param port_exists If zrythm port exists (when
 *   loading)
*/
static int
_create_port(Lv2Plugin*   lv2_plugin,
            uint32_t lv2_port_index,
            float    default_value,
            int            port_exists)
{
  Lv2Port* const lv2_port =
    &lv2_plugin->ports[lv2_port_index];

  lv2_port->lilv_port =
    lilv_plugin_get_port_by_index (
      lv2_plugin->lilv_plugin, lv2_port_index);
  lv2_port->sys_port  = NULL;
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
        calloc (AUDIO_ENGINE->block_length,
                sizeof (float));
    }
  else
    {
      char * port_name =
        g_strdup (lilv_node_as_string (sym));
      lv2_port->port = port_new (port_name);
      g_free (port_name);
      lv2_port->port->plugin = lv2_plugin->plugin;
      g_warn_if_fail (lv2_plugin->plugin->track);
      lv2_port->port->track =
        lv2_plugin->plugin->track;
      lv2_port->port->identifier.owner_type =
        PORT_OWNER_TYPE_PLUGIN;

      PortIdentifier * pi = &lv2_port->port_id;
      pi->plugin_slot = lv2_plugin->plugin->slot;
      pi->track_pos =
        lv2_plugin->plugin->track->pos;
      pi->owner_type =
        PORT_OWNER_TYPE_PLUGIN;
    }
  lv2_port->port->lv2_port = lv2_port;
  lv2_port->evbuf     = NULL;
  lv2_port->buf_size  = 0;
  lv2_port->index     = lv2_port_index;
  lv2_port->control   = 0.0f;

  const bool optional =
    lilv_port_has_property (
      lv2_plugin->lilv_plugin,
      lv2_port->lilv_port,
      PM_LILV_NODES.core_connectionOptional);

  /* Set the lv2_port flow (input or output) */
  if (lilv_port_is_a (
        lv2_plugin->lilv_plugin,
        lv2_port->lilv_port,
        PM_LILV_NODES.core_InputPort))
    {
      lv2_port->port->identifier.flow = FLOW_INPUT;
      lv2_port->port_id.flow = FLOW_INPUT;
    }
  else if (lilv_port_is_a (
             lv2_plugin->lilv_plugin,
             lv2_port->lilv_port,
             PM_LILV_NODES.core_OutputPort))
    {
      lv2_port->port->identifier.flow = FLOW_OUTPUT;
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
      lv2_port->port->identifier.type =
        TYPE_CONTROL;
      lv2_port->port_id.type = TYPE_CONTROL;
      lv2_port->control =
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
      lv2_port->port->identifier.type = TYPE_AUDIO;
      lv2_port->port_id.type = TYPE_AUDIO;
    }
  else if (lilv_port_is_a (
             lv2_plugin->lilv_plugin,
             lv2_port->lilv_port,
             PM_LILV_NODES.core_CVPort))
    {
      lv2_port->port->identifier.type = TYPE_CV;
      lv2_port->port_id.type = TYPE_CV;
    }
  else if (lilv_port_is_a (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.ev_EventPort))
    {
      lv2_port->port->identifier.type = TYPE_EVENT;
      lv2_port->port_id.type = TYPE_EVENT;
      lv2_port->port->midi_events =
        midi_events_new (lv2_port->port);
      lv2_port->old_api = true;
    }
  else if (lilv_port_is_a (
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.atom_AtomPort))
    {
      lv2_port->port->identifier.type = TYPE_EVENT;
      lv2_port->port_id.type = TYPE_EVENT;
      lv2_port->port->midi_events =
        midi_events_new (lv2_port->port);
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
      lv2_port->buf_size = lilv_node_as_int(min_size);
      buffer_size = MAX (
              buffer_size,
              lv2_port->buf_size * N_BUFFER_CYCLES);
    }
  lilv_node_free(min_size);

  return 0;
}

void
lv2_plugin_init_loaded (Lv2Plugin * lv2_plgn)
{
  for (int i = 0; i < lv2_plgn->num_ports; i++)
    lv2_plgn->ports[i].port =
      port_find_from_identifier (
        &lv2_plgn->ports[i].port_id);
}

/**
 * Set port structures from data (via create_port()) for all ports.
 *
 * Used when loading a project.
*/
int
lv2_set_ports(Lv2Plugin* lv2_plugin)
{
  float* default_values =
    (float*)calloc(
      lilv_plugin_get_num_ports (
        lv2_plugin->lilv_plugin),
        sizeof(float));
  lilv_plugin_get_port_ranges_float (
    lv2_plugin->lilv_plugin,
    NULL,
    NULL,
    default_values);

  int i;
  for (i = 0;
       i < lv2_plugin->num_ports; ++i)
    {
      if (_create_port (
            lv2_plugin, i,
            default_values[i], 1) < 0)
        {
          return -1;
        }
    }

  const LilvPort* control_input = lilv_plugin_get_port_by_designation (
          lv2_plugin->lilv_plugin,
          PM_LILV_NODES.core_InputPort,
          PM_LILV_NODES.core_control);
  if (control_input) {
          lv2_plugin->control_in = lilv_port_get_index (lv2_plugin->lilv_plugin,
                                                    control_input);
  }

  free(default_values);

  return 0;
}

/**
   Create port structures from data (via create_port()) for all ports.
*/
int
lv2_create_ports(Lv2Plugin* lv2_plugin)
{
  lv2_plugin->num_ports = lilv_plugin_get_num_ports(lv2_plugin->lilv_plugin);
  lv2_plugin->ports     = (Lv2Port*) calloc (lv2_plugin->num_ports,
                                           sizeof (Lv2Port));
  float* default_values = (float*)calloc(
                    lilv_plugin_get_num_ports(lv2_plugin->lilv_plugin),
                    sizeof(float));
  lilv_plugin_get_port_ranges_float(lv2_plugin->lilv_plugin,
                                    NULL,
                                    NULL,
                                    default_values);

  int i;
  for (i = 0; i < lv2_plugin->num_ports; ++i)
    {
      if (_create_port (
            lv2_plugin,
            i, default_values[i], 0) < 0)
        {
          return -1;
        }
    }

  const LilvPort* control_input = lilv_plugin_get_port_by_designation (
          lv2_plugin->lilv_plugin,
          PM_LILV_NODES.core_InputPort,
          PM_LILV_NODES.core_control);
  if (control_input) {
          lv2_plugin->control_in = lilv_port_get_index (lv2_plugin->lilv_plugin,
                                                    control_input);
  }

  free(default_values);

  return 0;
}

/**
   Allocate port buffers (only necessary for MIDI).
*/
void
lv2_allocate_port_buffers(Lv2Plugin* plugin)
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
                buf_size,
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
              i,
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
 * This function MUST set size and type appropriately.
 */
static const void *
lv2_get_port_value (const char * port_sym,
                    void       * user_data,
                    uint32_t   * size,
                    uint32_t   * type)
{
  Lv2Plugin * lv2_plugin = (Lv2Plugin *) user_data;

  Lv2Port * port = lv2_port_by_symbol (lv2_plugin,
                                        port_sym);
  *size = 0;
  *type = 0;

  if (port)
    {
      *size = sizeof (float);
      *type = PM_URIDS.atom_Float;
      return (const void *) &port->control;
    }

  return NULL;
}

/**
 * Function to set a port value. TODO
 *
 * Used when retrieving the state.
 * FIXME is this needed?
 */
/*static void*/
/*lv2_set_port_value (const char * port_sym,*/
                    /*void       * user_data,*/
                    /*const void * value,*/
                    /*uint32_t   * size,*/
                    /*uint32_t   * type)*/
/*{*/

/*}*/

/**
   Get a port structure by symbol.

   TODO: Build an index to make this faster, currently O(n) which may be
   a problem when restoring the state of plugins with many ports.
*/
Lv2Port*
lv2_port_by_symbol (
  Lv2Plugin* plugin,
  const char* sym)
{
	for (int i = 0; i < plugin->num_ports; ++i)
    {
      Lv2Port* const port =
        &plugin->ports[i];
      const LilvNode * port_sym =
        lilv_port_get_symbol (
          plugin->lilv_plugin,
          port->lilv_port);

      if (!strcmp (lilv_node_as_string (
                    port_sym), sym))
        return port;
	}

	return NULL;
}

Lv2Control*
lv2_control_by_symbol (
  Lv2Plugin * plugin,
  const char* sym)
{
	for (int i = 0;
       i < plugin->controls.n_controls; ++i)
    {
		if (!strcmp(lilv_node_as_string(plugin->controls.controls[i]->symbol),
		            sym)) {
			return plugin->controls.controls[i];
		}
	}
	return NULL;
}

static void
_print_control_value(Lv2Plugin* plugin, const Lv2Port* port, float value)
{
  const LilvNode* sym =
    lilv_port_get_symbol (
      plugin->lilv_plugin, port->lilv_port);
  g_message ("%s = %f",
             lilv_node_as_string(sym), value);
}

void
lv2_create_controls (
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
  LILV_FOREACH(nodes, p, properties)
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
 * Instantiates the Lv2Plugin UI.
 */
void
lv2_ui_instantiate (
  Lv2Plugin* plugin,
  const char* native_ui_type,
  void* parent)
{
  plugin->ui_host =
    suil_host_new (
      lv2_ui_write,
      lv2_ui_port_index, NULL, NULL);
  plugin->extuiptr = NULL;

  const char* bundle_uri =
    lilv_node_as_uri (
      lilv_ui_get_bundle_uri ( plugin->ui));
  const char* binary_uri =
    lilv_node_as_uri (
      lilv_ui_get_binary_uri (plugin->ui));
  char* bundle_path =
    lilv_file_uri_parse (bundle_uri, NULL);
  char* binary_path =
    lilv_file_uri_parse (binary_uri, NULL);

  const LV2_Feature data_feature = {
          LV2_DATA_ACCESS_URI, &plugin->ext_data
  };
  const LV2_Feature idle_feature = {
          LV2_UI__idleInterface, NULL
  };

  if (plugin->externalui)
    {
      const LV2_Feature external_lv_feature = {
        LV2_EXTERNAL_UI_DEPRECATED_URI, parent
      };
      const LV2_Feature external_kx_feature = {
        LV2_EXTERNAL_UI__Host, parent
      };
      const LV2_Feature instance_feature = {
        NS_EXT "instance-access",
        lilv_instance_get_handle(plugin->instance)
      };
      const LV2_Feature* ui_features[] = {
        &plugin->map_feature, &plugin->unmap_feature,
        &instance_feature,
        &data_feature,
        &idle_feature,
        &plugin->log_feature,
        &external_lv_feature,
        &external_kx_feature,
        &plugin->options_feature,
        NULL
      };

      plugin->ui_instance = suil_instance_new(
        plugin->ui_host,
        plugin,
        native_ui_type,
        lilv_node_as_uri(lilv_plugin_get_uri(plugin->lilv_plugin)),
        lilv_node_as_uri(lilv_ui_get_uri(plugin->ui)),
        lilv_node_as_uri(plugin->ui_type),
        bundle_path,
        binary_path,
        ui_features);

      if (plugin->ui_instance)
        {
          plugin->extuiptr =
            suil_instance_get_widget((SuilInstance*)plugin->ui_instance);
      }
      else
        {
          plugin->externalui = false;
        }
    }
  else
    {

      const LV2_Feature parent_feature = {
              LV2_UI__parent, parent
      };
      const LV2_Feature instance_feature = {
              NS_EXT "instance-access", lilv_instance_get_handle(plugin->instance)
      };
      const LV2_Feature* ui_features[] = {
              &plugin->map_feature, &plugin->unmap_feature,
              &instance_feature,
              &data_feature,
              &idle_feature,
              &plugin->log_feature,
              &parent_feature,
              &plugin->options_feature,
              NULL
      };

      plugin->ui_instance = suil_instance_new(
              plugin->ui_host,
              plugin,
              native_ui_type,
              lilv_node_as_uri(lilv_plugin_get_uri(plugin->lilv_plugin)),
              lilv_node_as_uri(lilv_ui_get_uri(plugin->ui)),
              lilv_node_as_uri(plugin->ui_type),
              bundle_path,
              binary_path,
              ui_features);
    }

  lilv_free(binary_path);
  lilv_free(bundle_path);

  if (!plugin->ui_instance)
    {
      g_warning ("Failed to get UI instance for %s",
                 plugin->plugin->descr->name);
    }
}

bool
lv2_ui_is_resizable(Lv2Plugin* plugin)
{
  if (!plugin->ui) {
          return false;
  }

  const LilvNode* s   = lilv_ui_get_uri(plugin->ui);
  LilvNode*       p   = lilv_new_uri(LILV_WORLD, LV2_CORE__optionalFeature);
  LilvNode*       fs  = lilv_new_uri(LILV_WORLD, LV2_UI__fixedSize);
  LilvNode*       nrs = lilv_new_uri(LILV_WORLD, LV2_UI__noUserResize);

  LilvNodes* fs_matches = lilv_world_find_nodes(LILV_WORLD, s, p, fs);
  LilvNodes* nrs_matches = lilv_world_find_nodes(LILV_WORLD, s, p, nrs);

  lilv_nodes_free(nrs_matches);
  lilv_nodes_free(fs_matches);
  lilv_node_free(nrs);
  lilv_node_free(fs);
  lilv_node_free(p);

  return !fs_matches && !nrs_matches;
}


/**
 * Write from ui
 */
void
lv2_ui_write(SuilController controller,
              uint32_t      port_index,
              uint32_t      buffer_size,
              uint32_t       protocol, ///< format
              const void*    buffer)
{
  Lv2Plugin* const plugin =
    (Lv2Plugin*)controller;

  if (protocol != 0 && protocol != PM_URIDS.atom_eventTransfer)
    {
      g_warning (
        "UI write with unsupported protocol %d (%s)",
        protocol,
        urid_unmap_uri (plugin, protocol));
      return;
    }

  if ((int) port_index >= plugin->num_ports)
    {
      g_warning (
        "UI write to out of range port index %d",
        port_index);
      return;
    }

  if (dump &&
      protocol == PM_URIDS.atom_eventTransfer)
    {
      const LV2_Atom* atom =
        (const LV2_Atom*)buffer;
      char * str  =
        sratom_to_turtle (
          plugin->sratom,
          &plugin->unmap,
          "plugin:",
          NULL, NULL,
          atom->type,
          atom->size,
          LV2_ATOM_BODY_CONST(atom));
      g_message (
        "## UI => Plugin (%u bytes) ##\n%s",
        atom->size, str);
      free(str);
    }

  char buf[sizeof(Lv2ControlChange) + buffer_size];
  Lv2ControlChange* ev = (Lv2ControlChange*)buf;
  ev->index    = port_index;
  ev->protocol = protocol;
  ev->size     = buffer_size;
  memcpy(ev->body, buffer, buffer_size);
  zix_ring_write(plugin->ui_events, buf, sizeof(buf));
}

void
lv2_apply_ui_events(Lv2Plugin* plugin, uint32_t nframes)
{
  if (!plugin->has_ui)
    {
      return;
    }

  Lv2ControlChange ev;
  const size_t space =
    zix_ring_read_space (plugin->ui_events);
  for (size_t i = 0; i < space;
       i += sizeof(ev) + ev.size)
    {
      zix_ring_read (
        plugin->ui_events,
        (char*)&ev, sizeof(ev));
      char body[ev.size];
      if (zix_ring_read (
            plugin->ui_events, body, ev.size) !=
          ev.size)
        {
          g_warning (
            "Error reading from UI ring buffer");
          break;
        }
      g_warn_if_fail (
        (int) ev.index < plugin->num_ports);
      Lv2Port* const port =
        &plugin->ports[ev.index];
      if (ev.protocol == 0)
        {
          assert(ev.size == sizeof(float));
          port->control = *(float*)body;
          /*g_message ("apply_ui_events %f",*/
                     /*port->control);*/
        }
      else if (ev.protocol ==
               PM_URIDS.atom_eventTransfer)
        {
          LV2_Evbuf_Iterator e =
            lv2_evbuf_end (port->evbuf);
          const LV2_Atom* const atom =
            (const LV2_Atom*)body;
          lv2_evbuf_write (
            &e, nframes, 0, atom->type, atom->size,
            (const uint8_t*)
              LV2_ATOM_BODY_CONST(atom));
        }
      else
        {
          g_warning ("error: Unknown control "
                     "change protocol %d",
                     ev.protocol);
        }
    }
}

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol)
{
	Lv2Plugin* const  plugin = (Lv2Plugin*)controller;
	Lv2Port* port = lv2_port_by_symbol(plugin, symbol);

	return port ? port->index : LV2UI_INVALID_PORT_INDEX;
}

void
lv2_init_ui(Lv2Plugin* plugin)
{
  // Set initial control port values
  for (int i = 0; i < plugin->num_ports; ++i)
    {
      if (plugin->ports[i].port->identifier.type ==
          TYPE_CONTROL)
        {
          lv2_gtk_ui_port_event (
            plugin, i,
            sizeof(float), 0,
            &plugin->ports[i].control);
        }
    }

  if (plugin->control_in != -1)
    {
      // Send patch:Get message for initial parameters/etc
      LV2_Atom_Forge forge = plugin->forge;
      LV2_Atom_Forge_Frame frame;
      uint8_t              buf[1024];
      lv2_atom_forge_set_buffer (
        &forge, buf, sizeof(buf));
      lv2_atom_forge_object (
        &forge, &frame, 0, PM_URIDS.patch_Get);

      const LV2_Atom* atom =
        lv2_atom_forge_deref(&forge, frame.ref);
      lv2_ui_write (
        plugin,
        plugin->control_in,
        lv2_atom_total_size (atom),
        PM_URIDS.atom_eventTransfer,
        atom);
      lv2_atom_forge_pop(&forge, &frame);
    }
}

bool
lv2_send_to_ui (
  Lv2Plugin*       plugin,
  uint32_t    port_index,
  uint32_t    type,
  uint32_t    size,
  const void* body)
{
  /* TODO: Be more disciminate about what to send */
  char evbuf[sizeof(Lv2ControlChange) + sizeof(LV2_Atom)];
  Lv2ControlChange* ev = (Lv2ControlChange*)evbuf;
  ev->index    = port_index;
  ev->protocol = PM_URIDS.atom_eventTransfer;
  ev->size     = sizeof(LV2_Atom) + size;

  LV2_Atom* atom = (LV2_Atom*)ev->body;
  atom->type = type;
  atom->size = size;

  if (zix_ring_write_space(plugin->plugin_events) >= sizeof(evbuf) + size) {
          zix_ring_write(plugin->plugin_events, evbuf, sizeof(evbuf));
          zix_ring_write(plugin->plugin_events, (const char*)body, size);
          return true;
  } else
    {
      PluginDescriptor * descr = ((Plugin *)plugin)->descr;
      g_warning ("lv2_send_to_ui: %s (%s) => UI buffer overflow!",
                 descr->name,
                 descr->uri);
      return false;
    }
}

bool
lv2_plugin_run (
  Lv2Plugin* plugin, uint32_t nframes)
{
    /* Read and apply control change events from UI */
  if (plugin->window)
    lv2_apply_ui_events(plugin, nframes);

  /* Run plugin for this cycle */
  lilv_instance_run(plugin->instance, nframes);

  /* Process any worker replies. */
  lv2_worker_emit_responses(&plugin->state_worker, plugin->instance);
  lv2_worker_emit_responses(&plugin->worker, plugin->instance);

  /* Notify the plugin the run() cycle is finished */
  if (plugin->worker.iface && plugin->worker.iface->end_run) {
          plugin->worker.iface->end_run(plugin->instance->lv2_handle);
  }

  /* Check if it's time to send updates to the UI */
  plugin->event_delta_t += nframes;
  bool     send_ui_updates = false;
  uint32_t update_frames   = AUDIO_ENGINE->sample_rate / plugin->ui_update_hz;
  if (plugin->has_ui && plugin->window && (plugin->event_delta_t > update_frames))
    {
      send_ui_updates = true;
      plugin->event_delta_t = 0;
    }

  return send_ui_updates;
}

int
lv2_plugin_update (gpointer data)
{
  Lv2Plugin * plugin = (Lv2Plugin *) data;
  /* Check quit flag and close if set. */
  if (zix_sem_try_wait(&plugin->exit_sem))
    {
      lv2_close_ui(plugin);
      return false;
    }

  /* Emit UI events. */
  if (plugin->window)
    {
      Lv2ControlChange ev;
      const size_t  space = zix_ring_read_space(plugin->plugin_events);
      for (size_t i = 0;
           i + sizeof(ev) < space;
           i += sizeof(ev) + ev.size)
        {
          /* Read event header to get the size */
          zix_ring_read(plugin->plugin_events, (char*)&ev, sizeof(ev));

          /* Resize read buffer if necessary */
          plugin->ui_event_buf = realloc(plugin->ui_event_buf, ev.size);
          void* const buf = plugin->ui_event_buf;

          /* Read event body */
          zix_ring_read(plugin->plugin_events, (char*)buf, ev.size);

          if (dump && ev.protocol ==
              PM_URIDS.atom_eventTransfer)
            {
              /* Dump event in Turtle to the
               * console */
              LV2_Atom * atom = (LV2_Atom *) buf;
              char * str =
                sratom_to_turtle (
                  plugin->ui_sratom,
                  &plugin->unmap,
                  "plugin:", NULL, NULL,
                  atom->type, atom->size,
                  LV2_ATOM_BODY(atom));
              g_message (
                "## Plugin => UI (%u bytes) ##"
                "\n%s\n", atom->size, str);
              free(str);
            }

          /*if (ev.index == 2)*/
            /*g_message ("lv2_gtk_ui_port_event from "*/
                       /*"lv2 update");*/
          lv2_gtk_ui_port_event (
            plugin, ev.index,
            ev.size, ev.protocol,
            ev.protocol == 0 ?
            (void *) &plugin->ports[ev.index].control :
            buf);

          if (ev.protocol == 0 && print_controls)
            {
              _print_control_value (
                plugin, &plugin->ports[ev.index],
                *(float*)buf);
            }
      }

      if (plugin->externalui && plugin->extuiptr) {
              LV2_EXTERNAL_UI_RUN(plugin->extuiptr);
      }
    }

  return TRUE;
}

static bool
_apply_control_arg(Lv2Plugin* plugin, const char* s)
{
  char  sym[256];
  float val = 0.0f;
  if (sscanf(s, "%[^=]=%f", sym, &val) != 2)
    {
      g_warning ("Ignoring invalid value `%s'\n", s);
      return false;
    }

  Lv2Control* control = lv2_control_by_symbol(plugin, sym);
  if (!control)
    {
      g_warning ("warning: Ignoring value for unknown control `%s'\n", sym);
      return false;
    }

  lv2_control_set_control (
    control, sizeof(float),
    PM_URIDS.atom_Float, &val);
  g_message ("%s = %f",
             sym, val);

  return true;
}

void
lv2_backend_activate_port(Lv2Plugin * lv2_plugin, uint32_t port_index)
{
  Lv2Port* const lv2_port =
    &lv2_plugin->ports[port_index];
  Port * port = lv2_port->port;

  /* Connect unsupported ports to NULL (known to be optional by this point) */
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
      lilv_instance_connect_port (
        lv2_plugin->instance,
        port_index, &lv2_port->control);
      break;
    case TYPE_AUDIO:
      /* already connected to Port */
      break;
    case TYPE_CV:
      /* already connected to port */
      break;
    case TYPE_EVENT:
      /* already connected to port */
      break;
    default:
      break;
    }
}

void
lv2_set_feature_data (Lv2Plugin * plugin)
{
  plugin->ext_data.data_access = NULL;

  plugin->map_feature.URI = LV2_URID__map;
  plugin->map_feature.data = NULL;
  plugin->unmap_feature.URI = LV2_URID__unmap;
  plugin->unmap_feature.data = NULL;
  plugin->make_path_feature.URI = LV2_STATE__makePath;
  plugin->make_path_feature.data = NULL;
  plugin->sched_feature.URI = LV2_WORKER__schedule;
  plugin->sched_feature.data = NULL;
  plugin->state_sched_feature.URI = LV2_WORKER__schedule;
  plugin->state_sched_feature.data  = NULL;
  plugin->safe_restore_feature.URI = LV2_STATE__threadSafeRestore;
  plugin->safe_restore_feature.data = NULL;
  plugin->log_feature.URI          = LV2_LOG__log;
  plugin->log_feature.data          = NULL;
  plugin->options_feature.URI      = LV2_OPTIONS__options;
  plugin->options_feature.data      = NULL;
  plugin->def_state_feature.URI    = LV2_STATE__loadDefaultState;
  plugin->def_state_feature.data    = NULL;

  /** These features have no data */
  plugin->buf_size_features[0].URI = LV2_BUF_SIZE__powerOf2BlockLength;
  plugin->buf_size_features[0].data = NULL;
  plugin->buf_size_features[1].URI = LV2_BUF_SIZE__fixedBlockLength;
  plugin->buf_size_features[1].data = NULL;;
  plugin->buf_size_features[2].URI = LV2_BUF_SIZE__boundedBlockLength;
  plugin->buf_size_features[2].data = NULL;;

  plugin->features[0] = &plugin->map_feature;
  plugin->features[1] = &plugin->unmap_feature;
  plugin->features[2] = &plugin->sched_feature;
  plugin->features[3] = &plugin->log_feature;
  plugin->features[4] = &plugin->options_feature;
  plugin->features[5] = &plugin->def_state_feature;
  plugin->features[6] = &plugin->safe_restore_feature;
  plugin->features[7] = &plugin->buf_size_features[0];
  plugin->features[8] = &plugin->buf_size_features[1];
  plugin->features[9] = &plugin->buf_size_features[2];
  plugin->features[10] = NULL;

  plugin->state_features[0] = &plugin->map_feature;
  plugin->state_features[1] = &plugin->unmap_feature;
  plugin->state_features[2] = &plugin->make_path_feature;
  plugin->state_features[3] = &plugin->state_sched_feature;
  plugin->state_features[4] = &plugin->safe_restore_feature;
  plugin->state_features[5] = &plugin->log_feature;
  plugin->state_features[6] = &plugin->options_feature;
  plugin->state_features[7] = NULL;
}

/**
 * Returns a newly allocated plugin descriptor for the given LilvPlugin
 * if it can be hosted, otherwise NULL.
 */
PluginDescriptor *
lv2_create_descriptor_from_lilv (const LilvPlugin * lp)
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
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_AudioPort, NULL);
  pd->num_midi_ins =
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.ev_EventPort, NULL)
    + count_midi_in;
  pd->num_audio_outs =
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.core_AudioPort, NULL);
  pd->num_midi_outs =
    lilv_plugin_get_num_ports_of_class(
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.ev_EventPort, NULL)
    + count_midi_out;
  pd->num_ctrl_ins =
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_ControlPort,
      NULL);
  pd->num_ctrl_outs =
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_OutputPort,
      PM_LILV_NODES.core_ControlPort,
      NULL);
  pd->num_cv_ins =
    lilv_plugin_get_num_ports_of_class (
      lp, PM_LILV_NODES.core_InputPort,
      PM_LILV_NODES.core_CVPort,
      NULL);
  pd->num_cv_outs =
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
lv2_create_from_uri (
  Plugin    * pl,
  const char * uri)
{
  LilvNode * lv2_uri =
    lilv_new_uri (LILV_WORLD, uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (
      PM_LILV_NODES.lilv_plugins,
      lv2_uri);

  if (!lilv_plugin)
    {
      g_error (
        "Failed to get LV2 Plugin from URI %s",
        uri);
      return NULL;
    }
  Lv2Plugin * lv2_plugin = lv2_new (pl);

  lv2_plugin->lilv_plugin = lilv_plugin;

  return lv2_plugin;
}

/**
 * Creates a new LV2 plugin using the given
 * Plugin instance.
 *
 * The given Plugin instance must be a newly
 * allocated one.
 */
Lv2Plugin *
lv2_new (Plugin *plugin)
{
  Lv2Plugin * lv2_plugin =
    (Lv2Plugin *) calloc (1, sizeof (Lv2Plugin));

  /* set pointers to each other */
  lv2_plugin->plugin = plugin;
  plugin->lv2 = lv2_plugin;

  return lv2_plugin;
}

void
lv2_free (Lv2Plugin * plugin)
{
  free (plugin);
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
 */
int
lv2_instantiate (
  Lv2Plugin * self,
  char *      preset_uri)
{
  Plugin * plugin = self->plugin;
  int i;

  lv2_set_feature_data (self);

  zix_sem_init(&self->work_lock, 1);

  self->map.handle = self;
  self->map.map = urid_map_uri;
  self->map_feature.data = &self->map;

  self->worker.plugin = self;
  self->state_worker.plugin = self;

  self->unmap.handle = self;
  self->unmap.unmap = urid_unmap_uri;
  self->unmap_feature.data = &self->unmap;

  lv2_atom_forge_init (&self->forge, &self->map);

  self->env = serd_env_new(NULL);
  serd_env_set_prefix_from_strings (
    self->env,
    (const uint8_t*)"patch",
    (const uint8_t*)LV2_PATCH_PREFIX);
  serd_env_set_prefix_from_strings(
          self->env, (const uint8_t*)"time", (const uint8_t*)LV2_TIME_PREFIX);
  serd_env_set_prefix_from_strings(
          self->env, (const uint8_t*)"xsd", (const uint8_t*)NS_XSD);

  self->sratom    = sratom_new(&self->map);
  self->ui_sratom = sratom_new(&self->map);
  sratom_set_env(self->sratom, self->env);
  sratom_set_env(self->ui_sratom, self->env);

#ifdef _WIN32
  self->temp_dir = lv2_strdup("self->XXXXX");
  _mktemp(self->temp_dir);
#else
  char* templ = lv2_strdup("/tmp/self->XXXXXX");
  self->temp_dir = lv2_strjoin(mkdtemp(templ), "/");
  free(templ);
#endif

  self->make_path.handle = &self;
  self->make_path.path = lv2_make_path;
  self->make_path_feature.data = &self->make_path;

  self->sched.handle = &self->worker;
  self->sched.schedule_work = lv2_worker_schedule;
  self->sched_feature.data = &self->sched;

  self->ssched.handle = &self->state_worker;
  self->ssched.schedule_work = lv2_worker_schedule;
  self->state_sched_feature.data = &self->ssched;

  self->llog.handle = self;
  self->llog.printf = lv2_printf;
  self->llog.vprintf = lv2_vprintf;
  self->log_feature.data = &self->llog;

  zix_sem_init(&self->exit_sem, 0);
  self->done = &self->exit_sem;

  zix_sem_init(&self->worker.sem, 0);

  /* Load preset, if specified */
  if (preset_uri)
    {
      LilvNode* preset = lilv_new_uri (LILV_WORLD,
                                      preset_uri);

      lv2_load_presets(self, NULL, NULL);
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
      char * state_file_path =
        g_build_filename (
          PROJECT->states_dir,
          self->state_file,
          NULL);
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
      self->lilv_plugin =
        lilv_plugins_get_by_uri (
          PM_LILV_NODES.lilv_plugins,
          lv2_uri);
      lilv_node_free (lv2_uri);
      if (!self->lilv_plugin)
        {
          g_warning ("Failed to find plugin");
        }

      /* Set default values for all ports */
      if (lv2_set_ports (self) < 0)
        {
          return -1;
        }

      self->control_in = (uint32_t)-1;
    }
  else
    {
      /* Set default values for all ports */
      if (lv2_create_ports (self) < 0)
        {
          return -1;
        }

      /* Set the zrythm plugin ports */
      for (i = 0; i < self->num_ports; ++i)
        {
          Lv2Port * lv2_port = &self->ports[i];
          Port * port = lv2_port->port;
          port->plugin = plugin;
          port->identifier.plugin_slot =
            plugin->slot;
          if (port->identifier.flow == FLOW_INPUT)
            {
              port->identifier.port_index =
                plugin->num_in_ports;
              plugin->in_ports[
                plugin->num_in_ports] = port;
              port_identifier_copy (
                &port->identifier,
                &plugin->in_port_ids[
                  plugin->num_in_ports]);
              plugin->num_in_ports++;
            }
          else if (port->identifier.flow ==
                   FLOW_OUTPUT)
            {
              port->identifier.port_index =
                plugin->num_out_ports;
              plugin->out_ports[
                plugin->num_out_ports] = port;
              port_identifier_copy (
                &port->identifier,
                &plugin->out_port_ids[
                  plugin->num_out_ports]);
              plugin->num_out_ports++;
            }
        }

      self->control_in = (uint32_t)-1;
    }

  /* Check that any required features are supported */
  LilvNodes* req_feats =
    lilv_plugin_get_required_features (
      self->lilv_plugin);
  LILV_FOREACH(nodes, f, req_feats)
    {
      const char* uri =
        lilv_node_as_uri (
          lilv_nodes_get (req_feats, f));
      if (!_feature_is_supported (self, uri))
        {
          g_warning ("Feature %s is not supported\n", uri);
          lilv_nodes_free (req_feats);
          return -1;
        }
    }
  lilv_nodes_free (req_feats);

  /* Check for thread-safe state restore() method. */
  if (lilv_plugin_has_feature (
        self->lilv_plugin,
        PM_LILV_NODES.state_threadSafeRestore))
    {
      self->safe_restore = true;
    }

  if (!self->state)
    {
      /* Not restoring state, load the self->as a preset to get default */
      self->state =
        lilv_state_new_from_world(
          LILV_WORLD, &self->map,
          lilv_plugin_get_uri (self->lilv_plugin));
    }

  /* Get a self->UI */
  g_message ("Looking for supported UI...");
  self->uis =
    lilv_plugin_get_uis (self->lilv_plugin);
  if (!PM_LILV_NODES.opts.generic_ui)
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
                this_ui,
                suil_ui_supported,
                PM_LILV_NODES.ui_Gtk3UI,
                &self->ui_type))
            {
              /* TODO: Multiple UI support */
              g_message ("UI is supported");
              self->ui = this_ui;
              break;
            }
          else
            {
              g_message ("UI unsupported by suil");
            }
        }
    }
  else if (!PM_LILV_NODES.opts.generic_ui && PM_LILV_NODES.opts.show_ui)
    {
      self->ui = lilv_uis_get(self->uis, lilv_uis_begin(self->uis));
    }

  if (!self->ui)
    {
      g_message (
        "Native UI not found, looking for external");
      LILV_FOREACH(uis, u, self->uis)
        {
          const LilvUI* ui = lilv_uis_get(self->uis, u);
          const LilvNodes* types = lilv_ui_get_classes(ui);
          LILV_FOREACH(nodes, t, types)
            {
              const char * pt = lilv_node_as_uri (
                              lilv_nodes_get(types, t));
              if (!strcmp (pt, "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget"))
                {
                  g_message ("Found UI: %s", pt);
                  g_message ("External KX UI selected");
                  self->externalui = true;
                  self->ui = ui;
                  self->ui_type =
                    PM_LILV_NODES.ui_externalkx;
                }
              else if (!strcmp (pt, "http://lv2plug.in/ns/extensions/ui#external"))
                {
                  g_message ("Found UI: %s", pt);
                  g_message ("External UI selected");
                  self->externalui = true;
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
      g_message ("Selected UI:           %s",
        lilv_node_as_uri (
          lilv_ui_get_uri(self->ui)));
    }
  else
    {
      g_message ("Selected UI:           None");
    }

  /* Create port and control structures */
  lv2_create_controls(self, true);
  lv2_create_controls(self, false);

  if (PM_LILV_NODES.opts.buffer_size == 0)
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
      PM_LILV_NODES.opts.buffer_size =
        AUDIO_ENGINE->midi_buf_size *
        N_BUFFER_CYCLES;
  }

  if (PM_LILV_NODES.opts.update_rate == 0.0)
    {
      /* Calculate a reasonable UI update frequency. */
      /*self->ui_update_hz = (float)AUDIO_ENGINE->sample_rate /*/
        /*AUDIO_ENGINE->midi_buf_size * 2.0f;*/
      /*self->ui_update_hz = MAX(25.0f, self->ui_update_hz);*/

      self->ui_update_hz =
        gdk_monitor_get_refresh_rate (
          gdk_display_get_monitor_at_window (
            gdk_display_get_default (),
            gtk_widget_get_window (
              GTK_WIDGET (MAIN_WINDOW))));
    }
  else
    {
      /* Use user-specified UI update rate. */
      self->ui_update_hz =
        PM_LILV_NODES.opts.update_rate;
      self->ui_update_hz = MAX(1.0f, self->ui_update_hz);
    }

  /* The UI can only go so fast, clamp to reasonable limits */
  self->ui_update_hz     = MIN(60, self->ui_update_hz);
  PM_LILV_NODES.opts.buffer_size = MAX(4096, PM_LILV_NODES.opts.buffer_size);
  g_message ("Comm buffers: %d bytes", PM_LILV_NODES.opts.buffer_size);
  g_message ("Update rate:  %.01f Hz", self->ui_update_hz);

  static float f_samplerate = 0.f;

  f_samplerate =
    AUDIO_ENGINE->sample_rate;

  /* Build options array to pass to plugin */
  const LV2_Options_Option options[] =
    {
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.param_sampleRate,
        sizeof(float),
        PM_URIDS.atom_Float,
        &f_samplerate },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.bufsz_minBlockLength,
        sizeof(int32_t),
        PM_URIDS.atom_Int,
        &AUDIO_ENGINE->block_length },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.bufsz_maxBlockLength,
        sizeof(int32_t),
        PM_URIDS.atom_Int,
        &AUDIO_ENGINE->block_length },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.bufsz_sequenceSize,
        sizeof(int32_t),
        PM_URIDS.atom_Int,
        &AUDIO_ENGINE->midi_buf_size },
      { LV2_OPTIONS_INSTANCE, 0,
        PM_URIDS.ui_updateRate,
        sizeof(float),
        PM_URIDS.atom_Float,
        &self->ui_update_hz },
      { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
    };
  memcpy(self->options,
         options,
         sizeof (self->options));

  /*self->options_feature.data =*/
    /*&self->options;*/
	init_feature (
    &self->options_feature,
	  LV2_OPTIONS__options,
	  (void*)self->options);

  /* Create Plugin <=> UI communication buffers */
  self->ui_events     = zix_ring_new(PM_LILV_NODES.opts.buffer_size);
  self->plugin_events = zix_ring_new(PM_LILV_NODES.opts.buffer_size);
  zix_ring_mlock(self->ui_events);
  zix_ring_mlock(self->plugin_events);

  /* Instantiate the plugin */
  self->instance =
    lilv_plugin_instantiate (
      self->lilv_plugin,
      AUDIO_ENGINE->sample_rate,
      self->features);
  if (!self->instance)
    {
      g_warning ("Failed to instantiate self->");
      return -1;
    }
  g_message ("Lilv plugin instantiated");

  self->ext_data.data_access =
    lilv_instance_get_descriptor (
      self->instance)->extension_data;

  lv2_allocate_port_buffers(self);

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

  /* Apply loaded state to self->instance if
   * necessary */
  if (self->state)
    {
      lv2_apply_state(self, self->state);
    }

  if (PM_LILV_NODES.opts.controls)
    {
      for (char** c = PM_LILV_NODES.opts.controls;
           *c; ++c)
        _apply_control_arg(self, *c);
    }

  /* Create Jack ports and connect self->ports
   * to buffers */
  for (i = 0; i < self->num_ports; ++i) {
      lv2_backend_activate_port(self, i);
  }

  /* Print initial control values */
  if (print_controls)
    for (i = 0; i < self->controls.n_controls; ++i)
      {
        Lv2Control* control =
          self->controls.controls[i];
        if (control->type == PORT)
          {
            Lv2Port* port =
              &self->ports[control->index];
            _print_control_value (
              self, port, port->control);
          }
      }

  /* Activate self->*/
  g_message ("Activating instance...");
  lilv_instance_activate(self->instance);
  g_message ("Instance activated");

  /* Discover UI */
  self->has_ui = lv2_discover_ui(self);

  /* Run UI (or prompt at console) */
//  g_message ("Opening UI...");
//  lv2_open_ui(self,NULL,NULL);
  /*g_message ("UI opened");*/

  return 0;
}

void
lv2_plugin_process (Lv2Plugin * lv2_plugin)
{
  int nframes = AUDIO_ENGINE->nframes;
#ifdef HAVE_JACK
  jack_client_t * client = AUDIO_ENGINE->client;
#endif
  int i, p;

  /* If transport state is not as expected, then something has changed */
  const bool xport_changed = (
    lv2_plugin->rolling != (TRANSPORT->play_state == PLAYSTATE_ROLLING) ||
    lv2_plugin->pos.frames != PLAYHEAD.frames ||
    lv2_plugin->bpm != TRANSPORT->bpm);

  uint8_t   pos_buf[256];
  LV2_Atom * lv2_pos = (LV2_Atom*) pos_buf;
  if (xport_changed)
    {
      /* Build an LV2 position object to report change to
       * plugin */
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
        PLAYHEAD.frames);
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
        (float) PLAYHEAD.beats - 1 +
        ((float) PLAYHEAD.ticks /
          (float) TRANSPORT->ticks_per_beat));
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_bar);
      lv2_atom_forge_long (
        forge,
        PLAYHEAD.bars - 1);
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
        TRANSPORT->beats_per_bar);
      lv2_atom_forge_key (
        forge,
        PM_URIDS.time_beatsPerMinute);
      lv2_atom_forge_float (forge, TRANSPORT->bpm);
    }

  /* Update transport state to expected values for next cycle */
  lv2_plugin->pos.frames =
    TRANSPORT->play_state == PLAYSTATE_ROLLING ?
    PLAYHEAD.frames + nframes :
    lv2_plugin->pos.frames;
  lv2_plugin->bpm      = TRANSPORT->bpm;
  lv2_plugin->rolling  = TRANSPORT->play_state == PLAYSTATE_ROLLING;

    /*switch (TRANSPORT->play_state) {*/
    /*case PLAYSTATE_PAUSE_REQUESTED:*/
            /*[>jalv->play_state = JALV_PAUSED;<]*/
            /*[>zix_sem_post(&jalv->paused);<]*/
            /*break;*/
    /*case JALV_PAUSED:*/
      /*for (uint32_t p = 0; p < lv2_plugin->num_ports; ++p)*/
        /*{*/
          /*jack_port_t* jport = jalv->ports[p].sys_port;*/
          /*if (jport && jalv->ports[p].flow == FLOW_OUTPUT)*/
            /*{*/
              /*void* buf = jack_port_get_buffer(jport, nframes);*/
              /*if (jalv->ports[p].type == TYPE_EVENT) {*/
                  /*jack_midi_clear_buffer(buf);*/
              /*} else {*/
                  /*memset(buf, '\0', nframes * sizeof(float));*/
              /*}*/
            /*}*/
        /*}*/
      /*return 0;*/
    /*default:*/
            /*break;*/
    /*}*/

  /* Prepare port buffers */
  for (p = 0; p < lv2_plugin->num_ports; ++p)
    {
      Lv2Port * lv2_port = &lv2_plugin->ports[p];
      Port * port = lv2_port->port;
      if (port->identifier.type == TYPE_AUDIO)
        {
          /* Connect lv2 ports  to plugin port buffers */
          /*port->nframes = nframes;*/
          lilv_instance_connect_port (
            lv2_plugin->instance, p, port->buf);
        }
      else if (port->identifier.type == TYPE_CV)
        {
          /* Connect plugin port directly to a
           * CV buffer in the port.  according to
           * the docs it has the same size as an
           * audio port. */
          lilv_instance_connect_port (
            lv2_plugin->instance,
            p, port->buf);
        }
      else if (port->identifier.type == TYPE_EVENT &&
               port->identifier.flow == FLOW_INPUT)
        {
          lv2_evbuf_reset(lv2_port->evbuf, true);

          /* Write transport change event if applicable */
          LV2_Evbuf_Iterator iter =
            lv2_evbuf_begin(lv2_port->evbuf);
          if (xport_changed)
            {
              lv2_evbuf_write (
                &iter, 0, 0,
                lv2_pos->type, lv2_pos->size,
                (const uint8_t*) LV2_ATOM_BODY(lv2_pos));
            }

          if (lv2_plugin->request_update)
            {
              /* Plugin state has changed, request an
               * update */
              const LV2_Atom_Object get = {
                      { sizeof(LV2_Atom_Object_Body),
                        PM_URIDS.atom_Object },
                      { 0, PM_URIDS.patch_Get } };
              lv2_evbuf_write (
                &iter, 0, 0,
                get.atom.type, get.atom.size,
                (const uint8_t*) LV2_ATOM_BODY(&get));
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
                  /*g_message ("plugin event %d", i);*/
                  /*g_message ("buf time %u %hhx %hhx %hhx",*/
                             /*ev->time,*/
                             /*ev->raw_buffer[0],*/
                             /*ev->raw_buffer[1],*/
                             /*ev->raw_buffer[2]);*/
                  lv2_evbuf_write (
                    &iter,
                    ev->time, 0,
                    PM_URIDS.midi_MidiEvent,
                    3, ev->raw_buffer);
                }
            }
          /*g_message ("waiting to clear events");*/
          /*zix_sem_wait (*/
            /*&port->midi_events->access_sem);*/
          midi_events_clear (
            port->midi_events, 0);
          /*zix_sem_post (*/
            /*&port->midi_events->access_sem);*/
          /*g_message ("posted clear events");*/
      }
      else if (port->identifier.type == TYPE_EVENT)
        {
          /* Clear event output for plugin to write to */
          lv2_evbuf_reset(lv2_port->evbuf, false);
        }
    }
  lv2_plugin->request_update = false;

  /* Run plugin for this cycle */
  const bool send_ui_updates =
    lv2_plugin_run (lv2_plugin, nframes) &&
    !AUDIO_ENGINE->exporting &&
    lv2_plugin->plugin->ui_instantiated;

  /* Deliver MIDI output and UI events */
  for (p = 0; p < lv2_plugin->num_ports;
       ++p)
    {
      Lv2Port* const lv2_port =
        &lv2_plugin->ports[p];
      Port * port = lv2_port->port;
      if (port->identifier.flow == FLOW_OUTPUT &&
          port->identifier.type == TYPE_CONTROL &&
          lilv_port_has_property(
            lv2_plugin->lilv_plugin,
            lv2_port->lilv_port,
            PM_LILV_NODES.core_reportsLatency))
        {
          if (lv2_plugin->plugin_latency !=
              lv2_port->control)
            {
              lv2_plugin->plugin_latency =
                lv2_port->control;
#ifdef HAVE_JACK
              jack_recompute_total_latencies (
                client);
#endif
            }
        }
      else if (port->identifier.flow == FLOW_OUTPUT &&
               port->identifier.type == TYPE_EVENT)
          {
            void* buf = NULL;

            for (LV2_Evbuf_Iterator i =
                   lv2_evbuf_begin(lv2_port->evbuf);
                 lv2_evbuf_is_valid(i);
                 i = lv2_evbuf_next(i))
              {
                // Get event from LV2 buffer
                uint32_t frames, subframes,
                         type, size;
                uint8_t* body;
                lv2_evbuf_get (
                  i, &frames, &subframes,
                  &type, &size, &body);

                if (buf && type ==
                    PM_URIDS.
                      midi_MidiEvent)
                  {
                    /* Write MIDI event to port */
                    midi_events_add_event_from_buf (
                      lv2_port->port->midi_events,
                      frames, body, size);
                  }

                if (lv2_plugin->has_ui &&
                    lv2_plugin->window &&
                    !lv2_port->old_api)
                  {
                    /* Forward event to UI */
                    lv2_send_to_ui (
                      lv2_plugin, p, type,
                      size, body);
                  }
              }
          }
        else if (send_ui_updates &&
                 port->identifier.type == TYPE_CONTROL)
          {
            char buf[sizeof(Lv2ControlChange) +
              sizeof(float)];
            Lv2ControlChange* ev =
              (Lv2ControlChange*)buf;
            ev->index    = p;
            ev->protocol = 0;
            ev->size     = sizeof(float);
            *(float*)ev->body = lv2_port->control;
            /*g_message ("writing ui event %f",*/
                       /*lv2_port->control);*/
            if (zix_ring_write (
                  lv2_plugin->plugin_events,
                  buf,
                  sizeof(buf))
                < sizeof(buf))
              {
                PluginDescriptor * descr =
                  lv2_plugin->plugin->descr;
                g_warning ("lv2_plugin_process: %s (%s) => UI buffer overflow!",
                           descr->name,
                           descr->uri);
              }
          }
    }

}

void
lv2_cleanup (Lv2Plugin *lv2_plugin)
{
  /* Wait for finish signal from UI or signal handler */
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
  zix_ring_free(lv2_plugin->ui_events);
  zix_ring_free(lv2_plugin->plugin_events);
  for (LilvNode** n =
         (LilvNode**)&PM_LILV_NODES;
       *n; ++n) {
          lilv_node_free(*n);
  }
  suil_host_free(lv2_plugin->ui_host);
  sratom_free(lv2_plugin->sratom);
  sratom_free(lv2_plugin->ui_sratom);
  lilv_uis_free(lv2_plugin->uis);

  zix_sem_destroy(&lv2_plugin->exit_sem);

  remove(lv2_plugin->temp_dir);
  free(lv2_plugin->temp_dir);
  free(lv2_plugin->ui_event_buf);
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

  if (!state)
    {
      g_warn_if_reached ();
      return -1;
    }

  char * label =
    g_strdup_printf (
      "%s.ttl",
      lv2_plugin->plugin->descr->name);
  /* FIXME check for return value */
  int rc = lilv_state_save (LILV_WORLD,
                            &lv2_plugin->map,
                            &lv2_plugin->unmap,
                            state,
                            NULL,
                            dir,
                            label);
  if (rc)
    {
      g_warning ("Lilv save state failed");
      return -1;
    }
  char * tmp = g_path_get_basename (dir);
  lv2_plugin->state_file =
    g_build_filename (tmp,
                      label,
                      NULL);
  g_free (label);
  g_free (tmp);
  lilv_state_free (state);

  return 0;
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
