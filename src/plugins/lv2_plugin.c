/*
 * plugins/lv2_plugin.c - For single instances of LV2 Plugins
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file
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
#include <unistd.h>

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
#include "audio/transport.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2/control.h"
#include "plugins/lv2/symap.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/io.h"
#include "utils/string.h"

#include <gtk/gtk.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>
#include <lv2/lv2plug.in/ns/ext/event/event.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/ext/port-props/port-props.h>
#include <lv2/lv2plug.in/ns/ext/presets/presets.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

#include <lilv/lilv.h>
#include <sratom/sratom.h>
#include <suil/suil.h>

#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/worker.h"

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

static LV2_URID
_map_uri(LV2_URID_Map_Handle handle,
        const char*         uri)
{
  Lv2Plugin* plugin = (Lv2Plugin*)handle;
  zix_sem_wait(&plugin->symap_lock);
  const LV2_URID id = symap_map(plugin->symap, uri);
  zix_sem_post(&plugin->symap_lock);
  return id;
}

static const char*
_unmap_uri(LV2_URID_Unmap_Handle handle,
          LV2_URID              urid)
{
  Lv2Plugin* plugin = (Lv2Plugin*)handle;
  zix_sem_wait(&plugin->symap_lock);
  const char* uri = symap_unmap(plugin->symap, urid);
  zix_sem_post(&plugin->symap_lock);
  return uri;
}

/**
   Create a port structure from data description.  This is called before plugin
   and Jack instantiation.  The remaining instance-specific setup
   (e.g. buffers) is done later in activate_port().
*/
static int
_create_port(Lv2Plugin*   lv2_plugin,
            uint32_t lv2_port_index,
            float    default_value,
            int            port_exists) ///< if zrythm port exists (when loading)
{
  LV2_Port* const lv2_port = &lv2_plugin->ports[lv2_port_index];

  lv2_port->lilv_port = lilv_plugin_get_port_by_index(lv2_plugin->lilv_plugin, lv2_port_index);
  lv2_port->sys_port  = NULL;
  const LilvNode* sym = lilv_port_get_symbol(lv2_plugin->lilv_plugin, lv2_port->lilv_port);
  if (port_exists)
    {
      /*lv2_port->port->nframes = AUDIO_ENGINE->block_length;*/
      /*lv2_port->port->buf = calloc (AUDIO_ENGINE->block_length,*/
                                    /*sizeof (sample_t));*/
    }
  else
    {
      char * port_name =
        g_strdup_printf ("LV2: %s",
                         lilv_node_as_string (sym));
      lv2_port->port = port_new (port_name);
      g_free (port_name);
    }
  lv2_port->port->owner_pl = lv2_plugin->plugin;
  lv2_port->port->lv2_port = lv2_port;
  lv2_port->evbuf     = NULL;
  lv2_port->buf_size  = 0;
  lv2_port->index     = lv2_port_index;
  lv2_port->control   = 0.0f;

  const bool optional = lilv_port_has_property(
          lv2_plugin->lilv_plugin, lv2_port->lilv_port, lv2_plugin->nodes.lv2_connectionOptional);

  /* Set the lv2_port flow (input or output) */
  if (lilv_port_is_a(lv2_plugin->lilv_plugin, lv2_port->lilv_port, lv2_plugin->nodes.lv2_InputPort))
    {
      lv2_port->port->flow = FLOW_INPUT;
    }
  else if (lilv_port_is_a(lv2_plugin->lilv_plugin, lv2_port->lilv_port,
                            lv2_plugin->nodes.lv2_OutputPort))
    {
      lv2_port->port->flow = FLOW_OUTPUT;
    }
  else if (!optional)
    {
      /* FIXME why g_error doesnt work */
      g_warning ("Mandatory lv2_port at %d has unknown type (neither input nor output)",
               lv2_port_index);
      return -1;
    }

  /* Set control values */
  if (lilv_port_is_a (lv2_plugin->lilv_plugin,
                      lv2_port->lilv_port,
                      lv2_plugin->nodes.lv2_ControlPort))
    {
      lv2_port->port->type    = TYPE_CONTROL;
      lv2_port->control = isnan(default_value) ? 0.0f : default_value;
      if (show_hidden ||
          !lilv_port_has_property(lv2_plugin->lilv_plugin, lv2_port->lilv_port, lv2_plugin->nodes.pprops_notOnGUI))
        {
          lv2_add_control (&lv2_plugin->controls,
                          lv2_new_port_control(lv2_plugin,
                                               lv2_port->index));
        }
    }
  else if (lilv_port_is_a (lv2_plugin->lilv_plugin,
                           lv2_port->lilv_port,
                           lv2_plugin->nodes.lv2_AudioPort))
    {
      lv2_port->port->type = TYPE_AUDIO;
#ifdef HAVE_JACK_METADATA
    }
  else if (lilv_port_is_a (lv2_plugin->lilv_plugin,
                           lv2_port->lilv_port,
                           lv2_plugin->nodes.lv2_CVPort))
    {
      lv2_port->port->type = TYPE_CV;
#endif
    }
  else if (lilv_port_is_a (lv2_plugin->lilv_plugin,
                           lv2_port->lilv_port,
                           lv2_plugin->nodes.ev_EventPort))
    {
      lv2_port->port->type = TYPE_EVENT;
      lv2_port->old_api = true;
    }
  else if (lilv_port_is_a (lv2_plugin->lilv_plugin,
                           lv2_port->lilv_port,
                           lv2_plugin->nodes.atom_AtomPort))
    {
      lv2_port->port->type = TYPE_EVENT;
      /*if (lv2_port->port->flow == FLOW_INPUT)*/
        /*{*/
          /*g_message ("input MIDI port created");*/
        /*}*/
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
            lv2_plugin->nodes.rsz_minimumSize);
  if (min_size && lilv_node_is_int(min_size))
    {
      lv2_port->buf_size = lilv_node_as_int(min_size);
      buffer_size = MAX (
              buffer_size,
              lv2_port->buf_size * N_BUFFER_CYCLES);
    }
  lilv_node_free(min_size);

  /* Update longest symbol for aligned console printing */
  const size_t    len = strlen(lilv_node_as_string(sym));
  if (len > lv2_plugin->longest_sym)
    {
      lv2_plugin->longest_sym = len;
    }
  return 0;
}

/**
 * Set port structures from data (via create_port()) for all ports.
 *
 * Used when loading a project.
*/
int
lv2_set_ports(Lv2Plugin* lv2_plugin)
{
  float* default_values = (float*)calloc(
                    lilv_plugin_get_num_ports(lv2_plugin->lilv_plugin),
                    sizeof(float));
  lilv_plugin_get_port_ranges_float(lv2_plugin->lilv_plugin,
                                    NULL,
                                    NULL,
                                    default_values);

  for (uint32_t i = 0; i < lv2_plugin->num_ports; ++i)
    {
      if (_create_port(lv2_plugin, i, default_values[i], 1) < 0)
        {
          return -1;
        }
    }

  const LilvPort* control_input = lilv_plugin_get_port_by_designation (
          lv2_plugin->lilv_plugin,
          lv2_plugin->nodes.lv2_InputPort,
          lv2_plugin->nodes.lv2_control);
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
  lv2_plugin->ports     = (LV2_Port*) calloc (lv2_plugin->num_ports,
                                           sizeof (LV2_Port));
  float* default_values = (float*)calloc(
                    lilv_plugin_get_num_ports(lv2_plugin->lilv_plugin),
                    sizeof(float));
  lilv_plugin_get_port_ranges_float(lv2_plugin->lilv_plugin,
                                    NULL,
                                    NULL,
                                    default_values);

  for (uint32_t i = 0; i < lv2_plugin->num_ports; ++i)
    {
      if (_create_port(lv2_plugin, i, default_values[i], 0) < 0)
        {
          return -1;
        }
    }

  const LilvPort* control_input = lilv_plugin_get_port_by_designation (
          lv2_plugin->lilv_plugin,
          lv2_plugin->nodes.lv2_InputPort,
          lv2_plugin->nodes.lv2_control);
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
  for (uint32_t i = 0; i < plugin->num_ports; ++i)
    {
      LV2_Port* const lv2_port     = &plugin->ports[i];
      Port* port     = lv2_port->port;
      switch (port->type) {
      case TYPE_EVENT:
          {
              lv2_evbuf_free(lv2_port->evbuf);
              const size_t buf_size = (lv2_port->buf_size > 0)
                      ? lv2_port->buf_size
                      : AUDIO_ENGINE->midi_buf_size;
              lv2_port->evbuf = lv2_evbuf_new(
                      buf_size,
                      plugin->map.map(plugin->map.handle,
                                    lilv_node_as_string(plugin->nodes.atom_Chunk)),
                      plugin->map.map(plugin->map.handle,
                                    lilv_node_as_string(plugin->nodes.atom_Sequence)));
              lilv_instance_connect_port(
                      plugin->instance, i, lv2_evbuf_get_buffer(lv2_port->evbuf));
      }
      default: break;
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

  LV2_Port * port = lv2_port_by_symbol (lv2_plugin,
                                        port_sym);
  *size = 0;
  *type = 0;

  if (port)
    {
      *size = sizeof (float);
      *type = lv2_plugin->urids.atom_Float;
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
LV2_Port*
lv2_port_by_symbol(Lv2Plugin* plugin, const char* sym)
{
	for (uint32_t i = 0; i < plugin->num_ports; ++i) {
		LV2_Port* const port     = &plugin->ports[i];
		const LilvNode*    port_sym = lilv_port_get_symbol(plugin->lilv_plugin,
		                                                   port->lilv_port);

		if (!strcmp(lilv_node_as_string(port_sym), sym)) {
			return port;
		}
	}

	return NULL;
}

Lv2ControlID*
lv2_control_by_symbol(Lv2Plugin* plugin, const char* sym)
{
	for (size_t i = 0; i < plugin->controls.n_controls; ++i) {
		if (!strcmp(lilv_node_as_string(plugin->controls.controls[i]->symbol),
		            sym)) {
			return plugin->controls.controls[i];
		}
	}
	return NULL;
}

static void
_print_control_value(Lv2Plugin* plugin, const LV2_Port* port, float value)
{
  const LilvNode* sym = lilv_port_get_symbol(plugin->lilv_plugin, port->lilv_port);
  g_message ("%-*s = %f\n", plugin->longest_sym, lilv_node_as_string(sym), value);
}

void
lv2_create_controls(Lv2Plugin* lv2_plugin, bool writable)
{
  const LilvPlugin* plugin         = lv2_plugin->lilv_plugin;
  LilvWorld*        world          = LILV_WORLD;
  LilvNode*         patch_writable = lilv_new_uri(world, LV2_PATCH__writable);
  LilvNode*         patch_readable = lilv_new_uri(world, LV2_PATCH__readable);

  LilvNodes* properties = lilv_world_find_nodes(
          world,
          lilv_plugin_get_uri(plugin),
          writable ? patch_writable : patch_readable,
          NULL);
  LILV_FOREACH(nodes, p, properties)
    {
      const LilvNode* property = lilv_nodes_get(properties, p);
      Lv2ControlID*      record   = NULL;

      if (!writable && lilv_world_ask(world,
                                      lilv_plugin_get_uri(plugin),
                                      patch_writable,
                                      property))
        {
          // Find existing writable control
          for (size_t i = 0; i < lv2_plugin->controls.n_controls; ++i)
            {
                  if (lilv_node_equals(lv2_plugin->controls.controls[i]->node, property))
                    {
                      record              = lv2_plugin->controls.controls[i];
                      record->is_readable = true;
                      break;
                    }
            }

          if (record) {
                  continue;
          }
      }

      record = lv2_new_property_control (lv2_plugin,
                                                 property);
      if (writable) {
              record->is_writable = true;
      } else {
              record->is_readable = true;
      }

      if (record->value_type)
        {
          lv2_add_control(&lv2_plugin->controls, record);
      } else {
              fprintf(stderr, "Parameter <%s> has unknown value type, ignored\n",
                      lilv_node_as_string(record->node));
              free(record);
      }
  }
  lilv_nodes_free(properties);

  lilv_node_free(patch_readable);
  lilv_node_free(patch_writable);

}

void
lv2_ui_instantiate(Lv2Plugin* plugin, const char* native_ui_type, void* parent)
{
  plugin->ui_host = suil_host_new(lv2_ui_write, lv2_ui_port_index, NULL, NULL);
  plugin->extuiptr = NULL;

  const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(plugin->ui));
  const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(plugin->ui));
  char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
  char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

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
              uint32_t       port_index,
              uint32_t       buffer_size,
              uint32_t       protocol, ///< format
              const void*    buffer)
{
  Lv2Plugin* const plugin = (Lv2Plugin*)controller;

  if (protocol != 0 && protocol != plugin->urids.atom_eventTransfer)
    {
      g_warning ("UI write with unsupported protocol %d (%s)",
              protocol, _unmap_uri(plugin, protocol));
      return;
    }

  if (port_index >= plugin->num_ports)
    {
      g_warning ("UI write to out of range port index %d",
              port_index);
      return;
    }

  if (dump && protocol == plugin->urids.atom_eventTransfer)
    {
      const LV2_Atom* atom = (const LV2_Atom*)buffer;
      char*           str  = sratom_to_turtle(
              plugin->sratom, &plugin->unmap, "plugin:", NULL, NULL,
              atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
      lv2_ansi_start(stdout, 36);
      g_message ("## UI => Plugin (%u bytes) ##\n%s", atom->size, str);
      lv2_ansi_reset(stdout);
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
  const size_t  space = zix_ring_read_space(plugin->ui_events);
  for (size_t i = 0; i < space; i += sizeof(ev) + ev.size)
    {
      zix_ring_read(plugin->ui_events, (char*)&ev, sizeof(ev));
      char body[ev.size];
      if (zix_ring_read(plugin->ui_events, body, ev.size) != ev.size) {
              fprintf(stderr, "error: Error reading from UI ring buffer\n");
              break;
      }
      assert(ev.index < plugin->num_ports);
      LV2_Port* const port = &plugin->ports[ev.index];
      if (ev.protocol == 0) {
              assert(ev.size == sizeof(float));
              port->control = *(float*)body;
        }
      else if (ev.protocol == plugin->urids.atom_eventTransfer)
        {
          LV2_Evbuf_Iterator    e    = lv2_evbuf_end(port->evbuf);
          const LV2_Atom* const atom = (const LV2_Atom*)body;
          lv2_evbuf_write(&e, nframes, 0, atom->type, atom->size,
                          (const uint8_t*)LV2_ATOM_BODY_CONST(atom));
        }
      else
        {
          g_warning ("error: Unknown control change protocol %d",
                     ev.protocol);
        }
    }
}

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol)
{
	Lv2Plugin* const  plugin = (Lv2Plugin*)controller;
	LV2_Port* port = lv2_port_by_symbol(plugin, symbol);

	return port ? port->index : LV2UI_INVALID_PORT_INDEX;
}

void
lv2_init_ui(Lv2Plugin* plugin)
{
  // Set initial control port values
  for (uint32_t i = 0; i < plugin->num_ports; ++i)
    {
      if (plugin->ports[i].port->type == TYPE_CONTROL) {
              lv2_ui_port_event(plugin, i,
                                 sizeof(float), 0,
                                 &plugin->ports[i].control);
      }
    }

  if (plugin->control_in != (uint32_t)-1) {
          // Send patch:Get message for initial parameters/etc
          LV2_Atom_Forge       forge = plugin->forge;
          LV2_Atom_Forge_Frame frame;
          uint8_t              buf[1024];
          lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));
          lv2_atom_forge_object(&forge, &frame, 0, plugin->urids.patch_Get);

          const LV2_Atom* atom = lv2_atom_forge_deref(&forge, frame.ref);
          lv2_ui_write(plugin,
                        plugin->control_in,
                        lv2_atom_total_size(atom),
                        plugin->urids.atom_eventTransfer,
                        atom);
          lv2_atom_forge_pop(&forge, &frame);
  }
}

bool
lv2_send_to_ui(Lv2Plugin*       plugin,
                uint32_t    port_index,
                uint32_t    type,
                uint32_t    size,
                const void* body)
{
  /* TODO: Be more disciminate about what to send */
  char evbuf[sizeof(Lv2ControlChange) + sizeof(LV2_Atom)];
  Lv2ControlChange* ev = (Lv2ControlChange*)evbuf;
  ev->index    = port_index;
  ev->protocol = plugin->urids.atom_eventTransfer;
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
lv2_run(Lv2Plugin* plugin, uint32_t nframes)
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

bool
lv2_update(Lv2Plugin* plugin)
{
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

          if (dump && ev.protocol == plugin->urids.atom_eventTransfer) {
                  /* Dump event in Turtle to the console */
                  LV2_Atom* atom = (LV2_Atom*)buf;
                  char*     str  = sratom_to_turtle(
                          plugin->ui_sratom, &plugin->unmap, "plugin:", NULL, NULL,
                          atom->type, atom->size, LV2_ATOM_BODY(atom));
                  lv2_ansi_start(stdout, 35);
                  printf("\n## Plugin => UI (%u bytes) ##\n%s\n", atom->size, str);
                  lv2_ansi_reset(stdout);
                  free(str);
          }

          if (plugin->ui_instance)
            {
              suil_instance_port_event(plugin->ui_instance, ev.index,
                                       ev.size, ev.protocol, buf);
            }
          else
            {
              lv2_ui_port_event(plugin, ev.index, ev.size, ev.protocol, buf);
            }

          if (ev.protocol == 0 && print_controls)
            {
              _print_control_value(plugin, &plugin->ports[ev.index], *(float*)buf);
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

  Lv2ControlID* control = lv2_control_by_symbol(plugin, sym);
  if (!control)
    {
      g_warning ("warning: Ignoring value for unknown control `%s'\n", sym);
      return false;
    }

  lv2_set_control(control, sizeof(float), plugin->urids.atom_Float, &val);
  g_message ("%-*s = %f\n", plugin->longest_sym, sym, val);

  return true;
}

void
lv2_backend_activate_port(Lv2Plugin * lv2_plugin, uint32_t port_index)
{
  LV2_Port* const lv2_port   = &lv2_plugin->ports[port_index];
  Port * port = lv2_port->port;

  /*const LilvNode* sym = lilv_port_get_symbol(lv2_plugin->lilv_plugin, lv2_port->lilv_port);*/

  /* Connect unsupported ports to NULL (known to be optional by this point) */
  if (port->flow == FLOW_UNKNOWN || port->type == TYPE_UNKNOWN)
    {
      lilv_instance_connect_port(lv2_plugin->instance, port_index, NULL);
      return;
    }

  /* Connect the port based on its type */
  switch (port->type) {
  case TYPE_CONTROL:
    lilv_instance_connect_port (lv2_plugin->instance,
                                port_index, &lv2_port->control);
    break;
  case TYPE_AUDIO:
    /*port->sys_port = jack_port_register(*/
            /*client, lilv_node_as_string(sym),*/
            /*JACK_DEFAULT_AUDIO_TYPE, jack_flags, 0);*/
    /* already connected to Port */
    break;
#ifdef HAVE_JACK_METADATA
  case TYPE_CV:
          lv2_port->sys_port = jack_port_register(
                  client, lilv_node_as_string(sym),
                  JACK_DEFAULT_AUDIO_TYPE, jack_flags, 0);
          if (lv2_port->sys_port) {
                  jack_set_property(client, jack_port_uuid(lv2_port->sys_port),
                                    "http://jackaudio.org/metadata/signal-type", "CV",
                                    "text/plain");
          }
          break;
#endif
  case TYPE_EVENT:
          if (lilv_port_supports_event(
                      lv2_plugin->lilv_plugin,
                      port->lv2_port->lilv_port,
                      lv2_plugin->nodes.midi_MidiEvent)) {
                  /*port->sys_port = jack_port_register(*/
                          /*client, lilv_node_as_string(sym),*/
                          /*JACK_DEFAULT_MIDI_TYPE, jack_flags, 0);*/
          }
          break;
  default:
          break;
  }

#ifdef HAVE_JACK_METADATA
  if (lv2_port->sys_port) {
          // Set port order to index
          char index_str[16];
          snprintf(index_str, sizeof(index_str), "%d", port_index);
          jack_set_property(client, jack_port_uuid(lv2_port->sys_port),
                            "http://jackaudio.org/metadata/order", index_str,
                            "http://www.w3.org/2001/XMLSchema#integer");

          // Set port pretty name to label
          LilvNode* name = lilv_port_get_name(lv2_plugin->lilv_plugin, lv2_port->lilv_port);
          jack_set_property(client, jack_port_uuid(lv2_port->sys_port),
                            JACK_METADATA_PRETTY_NAME, lilv_node_as_string(name),
                            "text/plain");
          lilv_node_free(name);
  }
#endif
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
  const LilvNode*  lv2_uri = lilv_plugin_get_uri (lp);
  const char * uri_str = lilv_node_as_string (lv2_uri);
  if (!string_is_ascii (uri_str))
    {
      g_warning ("Invalid plugin URI, skipping");
      return NULL;
    }

  LilvNode* name = lilv_plugin_get_name(lp);

  /* check if we can host the Plugin */
  if (!lv2_uri)
    {
      g_error ("Plugin URI not found, try lv2ls to get a list of all plugins");
      lilv_node_free (name);
      return NULL;
    }
  if (!name || !lilv_plugin_get_port_by_index(lp, 0))
    {
      g_message ("Ignoring invalid LV2 plugin %s",
                 lilv_node_as_string(lilv_plugin_get_uri(lp)));
      lilv_node_free(name);
      return NULL;
    }

  /* check if shared lib exists */
  const LilvNode * lib_uri = lilv_plugin_get_library_uri (lp);
  char * path = lilv_file_uri_parse (lilv_node_as_string (lib_uri),
                                     NULL);
  if (access (path, F_OK) == -1)
    {
      g_warning ("%s not found, skipping %s",
                 path, lilv_node_as_string (name));
      lilv_node_free(name);
      return NULL;
    }

  if (lilv_plugin_has_feature(lp, LV2_SETTINGS.lv2_inPlaceBroken))
    {
      g_warning ("Ignoring LV2 plugin \"%s\" since it cannot do inplace processing.",
                  lilv_node_as_string(name));
      lilv_node_free(name);
      return NULL;
    }

#ifdef HAVE_LV2_1_2_0
  LilvNodes *required_features = lilv_plugin_get_required_features (lp);
  if (lilv_nodes_contains (required_features,
                           LV2_SETTINGS.world->bufz_powerOf2BlockLength) ||
                  lilv_nodes_contains (required_features, LV2_SETTINGS.world->bufz_fixedBlockLength)
     )
    {
      g_warning ("Ignoring LV2 Plugin \"%1\" because its buffer-size requirements cannot be satisfied.",
               lilv_node_as_string(name));
      lilv_nodes_free (required_features);
      lilv_node_free (name);
      return NULL;
  }
  lilv_nodes_free(required_features);
#endif

  /* set descriptor info */
  PluginDescriptor * pd = calloc (1, sizeof (PluginDescriptor));
  pd->protocol = PROT_LV2;
  const char * str = lilv_node_as_string (name);
  pd->name = g_strdup (str);
  lilv_node_free (name);
  LilvNode * author = lilv_plugin_get_author_name (lp);
  str = lilv_node_as_string (author);
  pd->author = g_strdup (str);
  lilv_node_free (author);
  const LilvPluginClass* pclass = lilv_plugin_get_class(lp);
  const LilvNode * label  = lilv_plugin_class_get_label(pclass);
  str = lilv_node_as_string (label);
  pd->category = g_strdup (str);

  /* count atom-event-ports that feature
   * atom:supports <http://lv2plug.in/ns/ext/midi#MidiEvent>
   */
  int count_midi_out = 0;
  int count_midi_in = 0;
  for (uint32_t i = 0;
       i < lilv_plugin_get_num_ports (lp);
       ++i)
    {
      const LilvPort* port  = lilv_plugin_get_port_by_index (lp, i);
      if (lilv_port_is_a(lp, port, LV2_SETTINGS.atom_AtomPort))
        {
          LilvNodes* buffer_types = lilv_port_get_value(
                  lp, port, LV2_SETTINGS.atom_bufferType);
          LilvNodes* atom_supports = lilv_port_get_value(
                  lp, port, LV2_SETTINGS.atom_supports);

          if (lilv_nodes_contains (buffer_types, LV2_SETTINGS.atom_Sequence)
                && lilv_nodes_contains (atom_supports, LV2_SETTINGS.midi_MidiEvent))
            {
              if (lilv_port_is_a (lp, port, LV2_SETTINGS.lv2_InputPort))
                {
                  count_midi_in++;
                }
              if (lilv_port_is_a (lp, port, LV2_SETTINGS.lv2_OutputPort))
                {
                  count_midi_out++;
                }
            }
          lilv_nodes_free(buffer_types);
          lilv_nodes_free(atom_supports);
        }
    }

  pd->num_audio_ins =
          lilv_plugin_get_num_ports_of_class(
                  lp, LV2_SETTINGS.lv2_InputPort, LV2_SETTINGS.lv2_AudioPort, NULL);
  pd->num_midi_ins =
          lilv_plugin_get_num_ports_of_class(
                  lp, LV2_SETTINGS.lv2_InputPort, LV2_SETTINGS.ev_EventPort, NULL)
          + count_midi_in;

  pd->num_audio_outs =
          lilv_plugin_get_num_ports_of_class(
                  lp, LV2_SETTINGS.lv2_OutputPort, LV2_SETTINGS.lv2_AudioPort, NULL);
  pd->num_midi_outs =
          lilv_plugin_get_num_ports_of_class(
                  lp, LV2_SETTINGS.lv2_OutputPort, LV2_SETTINGS.ev_EventPort, NULL)
          + count_midi_out;
  pd->num_ctrl_ins =
    lilv_plugin_get_num_ports_of_class (lp, LV2_SETTINGS.lv2_InputPort,
                                        LV2_SETTINGS.lv2_ControlPort,
                                        NULL);
  pd->num_ctrl_outs =
    lilv_plugin_get_num_ports_of_class (lp, LV2_SETTINGS.lv2_InputPort,
                                        LV2_SETTINGS.lv2_ControlPort,
                                        NULL);

  pd->uri = g_strdup (uri_str);

  return pd;
}

/**
 * Creates an LV2 plugin from given uri.
 *
 * Used when populating the plugin browser.
 */
Lv2Plugin *
lv2_create_from_uri (Plugin    * plugin,  ///< a newly created plugin, with its descriptor filled in
                     const char * uri ///< the uri
                     )
{
  LilvNode * lv2_uri = lilv_new_uri (LILV_WORLD, uri);
  const LilvPlugin * lilv_plugin = lilv_plugins_get_by_uri (
                                    LV2_SETTINGS.lilv_plugins,
                                    lv2_uri);

  if (!lilv_plugin)
    {
      g_error ("Failed to get LV2 Plugin from URI %s", uri);
      return NULL;
    }
  Lv2Plugin * lv2_plugin = lv2_new (plugin);

  lv2_plugin->lilv_plugin = lilv_plugin;

  return lv2_plugin;
}

/**
 * Creates an LV2 plugin from state.
 *
 * Used when loading project files.
 */
Lv2Plugin *
lv2_create_from_state (Plugin    * plugin,  ///< a newly created plugin
                       const char * _path    ///< path for state to load
                             )
{
  /*Lv2Plugin * lv2_plugin = lv2_new (plugin);*/

  /*lv2_set_feature_data (lv2_plugin);*/

  /*[> Get plugin URI <]*/
  /*lv2_plugin->state    = NULL;*/
  /*LilvNode*  lv2_uri = NULL;*/

  /*struct stat info;*/
  /*stat (_path, &info);*/
  /*if (S_ISDIR(info.st_mode))*/
    /*{*/
      /*char* path = lv2_strjoin (_path, "/state.ttl");*/
      /*lv2_plugin->state = lilv_state_new_from_file(LILV_WORLD, &lv2_plugin->map, NULL, path);*/
      /*free(path);*/
    /*}*/
  /*else*/
    /*{*/
      /*lv2_plugin->state = lilv_state_new_from_file(LILV_WORLD,*/
                                       /*&lv2_plugin->map, NULL,*/
                                       /*_path);*/
    /*}*/
  /*if (!lv2_plugin->state)*/
    /*{*/
      /*g_error ("Failed to load state from %s\n", _path);*/
    /*}*/

  /*lv2_uri = lilv_node_duplicate(lilv_state_get_plugin_uri(lv2_plugin->state));*/

  /*if (!lv2_uri)*/
    /*{*/
          /*g_error ("Missing plugin URI, try lv2ls to list plugins\n");*/
    /*}*/

  /*[> Find plugin <]*/
  /*g_message ("Plugin:       %s\n", lilv_node_as_string(lv2_uri));*/
  /*lv2_plugin->lilv_plugin = lilv_plugins_get_by_uri(LV2_SETTINGS.lilv_plugins,*/
                                           /*lv2_uri);*/
  /*lilv_node_free(lv2_uri);*/
  /*if (!lv2_plugin->lilv_plugin) {*/
          /*g_error("Failed to find plugin\n");*/
  /*}*/

  /*[> set plugin descriptor <]*/
  /*set_descriptor (lv2_plugin);*/

  return NULL;
}

/**
 * Creates a new LV2 plugin using the given Plugin instance.
 *
 * The given plugin instance must be a newly allocated one.
 */
Lv2Plugin *
lv2_new (Plugin *plugin ///< a newly allocated plugin instance
         )
{
  Lv2Plugin * lv2_plugin = (Lv2Plugin *) calloc (1, sizeof (Lv2Plugin));

  /* set pointers to each other */
  lv2_plugin->plugin = plugin;
  plugin->original_plugin = lv2_plugin;

  return lv2_plugin;
}

void
lv2_free (Lv2Plugin * plugin)
{
  free (plugin);
}

/**
 * Instantiate the plugin.
 * TODO
 */
int
lv2_instantiate (Lv2Plugin      * lv2_plugin,   ///< plugin to instantiate
                 char            * preset_uri   ///< uri of preset to load
                )
{
  LilvWorld * world = LILV_WORLD;
  Plugin * plugin = lv2_plugin->plugin;

  lv2_set_feature_data (lv2_plugin);

  /* Cache URIs for concepts we'll use */
  lv2_plugin->nodes.atom_AtomPort          = lilv_new_uri(world, LV2_ATOM__AtomPort);
  lv2_plugin->nodes.atom_Chunk             = lilv_new_uri(world, LV2_ATOM__Chunk);
  lv2_plugin->nodes.atom_Float             = lilv_new_uri(world, LV2_ATOM__Float);
  lv2_plugin->nodes.atom_Path              = lilv_new_uri(world, LV2_ATOM__Path);
  lv2_plugin->nodes.atom_Sequence          = lilv_new_uri(world, LV2_ATOM__Sequence);
  lv2_plugin->nodes.ev_EventPort           = lilv_new_uri(world, LV2_EVENT__EventPort);
  lv2_plugin->nodes.lv2_AudioPort          = lilv_new_uri(world, LV2_CORE__AudioPort);
  lv2_plugin->nodes.lv2_CVPort             = lilv_new_uri(world, LV2_CORE__CVPort);
  lv2_plugin->nodes.lv2_ControlPort        = lilv_new_uri(world, LV2_CORE__ControlPort);
  lv2_plugin->nodes.lv2_InputPort          = lilv_new_uri(world, LV2_CORE__InputPort);
  lv2_plugin->nodes.lv2_OutputPort         = lilv_new_uri(world, LV2_CORE__OutputPort);
  lv2_plugin->nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
  lv2_plugin->nodes.lv2_control            = lilv_new_uri(world, LV2_CORE__control);
  lv2_plugin->nodes.lv2_default            = lilv_new_uri(world, LV2_CORE__default);
  lv2_plugin->nodes.lv2_enumeration        = lilv_new_uri(world, LV2_CORE__enumeration);
  lv2_plugin->nodes.lv2_integer            = lilv_new_uri(world, LV2_CORE__integer);
  lv2_plugin->nodes.lv2_maximum            = lilv_new_uri(world, LV2_CORE__maximum);
  lv2_plugin->nodes.lv2_minimum            = lilv_new_uri(world, LV2_CORE__minimum);
  lv2_plugin->nodes.lv2_name               = lilv_new_uri(world, LV2_CORE__name);
  lv2_plugin->nodes.lv2_reportsLatency     = lilv_new_uri(world, LV2_CORE__reportsLatency);
  lv2_plugin->nodes.lv2_sampleRate         = lilv_new_uri(world, LV2_CORE__sampleRate);
  lv2_plugin->nodes.lv2_symbol             = lilv_new_uri(world, LV2_CORE__symbol);
  lv2_plugin->nodes.lv2_toggled            = lilv_new_uri(world, LV2_CORE__toggled);
  lv2_plugin->nodes.midi_MidiEvent         = lilv_new_uri(world, LV2_MIDI__MidiEvent);
  lv2_plugin->nodes.pg_group               = lilv_new_uri(world, LV2_PORT_GROUPS__group);
  lv2_plugin->nodes.pprops_logarithmic     = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
  lv2_plugin->nodes.pprops_notOnGUI        = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
  lv2_plugin->nodes.pprops_rangeSteps      = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
  lv2_plugin->nodes.pset_Preset            = lilv_new_uri(world, LV2_PRESETS__Preset);
  lv2_plugin->nodes.pset_bank              = lilv_new_uri(world, LV2_PRESETS__bank);
  lv2_plugin->nodes.rdfs_comment           = lilv_new_uri(world, LILV_NS_RDFS "comment");
  lv2_plugin->nodes.rdfs_label             = lilv_new_uri(world, LILV_NS_RDFS "label");
  lv2_plugin->nodes.rdfs_range             = lilv_new_uri(world, LILV_NS_RDFS "range");
  lv2_plugin->nodes.rsz_minimumSize        = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
  lv2_plugin->nodes.work_interface         = lilv_new_uri(world, LV2_WORKER__interface);
  lv2_plugin->nodes.work_schedule          = lilv_new_uri(world, LV2_WORKER__schedule);
  lv2_plugin->nodes.ui_externallv          = lilv_new_uri(world, "http://lv2plug.in/ns/extensions/ui#external");
  lv2_plugin->nodes.ui_externalkx          = lilv_new_uri(world, "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget");
  lv2_plugin->nodes.end                    = NULL;


  lv2_plugin->symap = symap_new();
  zix_sem_init(&lv2_plugin->symap_lock, 1);
  zix_sem_init(&lv2_plugin->work_lock, 1);

  lv2_plugin->map.handle  = lv2_plugin;
  lv2_plugin->map.map     = _map_uri;
  lv2_plugin->map_feature.data = &lv2_plugin->map;

  lv2_plugin->worker.plugin      = lv2_plugin;
  lv2_plugin->state_worker.plugin = lv2_plugin;

  lv2_plugin->unmap.handle  = lv2_plugin;
  lv2_plugin->unmap.unmap   = _unmap_uri;
  lv2_plugin->unmap_feature.data = &lv2_plugin->unmap;

  lv2_atom_forge_init(&lv2_plugin->forge, &lv2_plugin->map);

  lv2_plugin->env = serd_env_new(NULL);
  serd_env_set_prefix_from_strings(
          lv2_plugin->env, (const uint8_t*)"patch", (const uint8_t*)LV2_PATCH_PREFIX);
  serd_env_set_prefix_from_strings(
          lv2_plugin->env, (const uint8_t*)"time", (const uint8_t*)LV2_TIME_PREFIX);
  serd_env_set_prefix_from_strings(
          lv2_plugin->env, (const uint8_t*)"xsd", (const uint8_t*)NS_XSD);

  lv2_plugin->sratom    = sratom_new(&lv2_plugin->map);
  lv2_plugin->ui_sratom = sratom_new(&lv2_plugin->map);
  sratom_set_env(lv2_plugin->sratom, lv2_plugin->env);
  sratom_set_env(lv2_plugin->ui_sratom, lv2_plugin->env);

  lv2_plugin->urids.atom_Float           = symap_map(lv2_plugin->symap, LV2_ATOM__Float);
  lv2_plugin->urids.atom_Int             = symap_map(lv2_plugin->symap, LV2_ATOM__Int);
  lv2_plugin->urids.atom_Object          = symap_map(lv2_plugin->symap, LV2_ATOM__Object);
  lv2_plugin->urids.atom_Path            = symap_map(lv2_plugin->symap, LV2_ATOM__Path);
  lv2_plugin->urids.atom_String          = symap_map(lv2_plugin->symap, LV2_ATOM__String);
  lv2_plugin->urids.atom_eventTransfer   = symap_map(lv2_plugin->symap, LV2_ATOM__eventTransfer);
  lv2_plugin->urids.bufsz_maxBlockLength = symap_map(lv2_plugin->symap, LV2_BUF_SIZE__maxBlockLength);
  lv2_plugin->urids.bufsz_minBlockLength = symap_map(lv2_plugin->symap, LV2_BUF_SIZE__minBlockLength);
  lv2_plugin->urids.bufsz_sequenceSize   = symap_map(lv2_plugin->symap, LV2_BUF_SIZE__sequenceSize);
  lv2_plugin->urids.log_Error            = symap_map(lv2_plugin->symap, LV2_LOG__Error);
  lv2_plugin->urids.log_Trace            = symap_map(lv2_plugin->symap, LV2_LOG__Trace);
  lv2_plugin->urids.log_Warning          = symap_map(lv2_plugin->symap, LV2_LOG__Warning);
  lv2_plugin->urids.midi_MidiEvent       = symap_map(lv2_plugin->symap, LV2_MIDI__MidiEvent);
  lv2_plugin->urids.param_sampleRate     = symap_map(lv2_plugin->symap, LV2_PARAMETERS__sampleRate);
  lv2_plugin->urids.patch_Get            = symap_map(lv2_plugin->symap, LV2_PATCH__Get);
  lv2_plugin->urids.patch_Put            = symap_map(lv2_plugin->symap, LV2_PATCH__Put);
  lv2_plugin->urids.patch_Set            = symap_map(lv2_plugin->symap, LV2_PATCH__Set);
  lv2_plugin->urids.patch_body           = symap_map(lv2_plugin->symap, LV2_PATCH__body);
  lv2_plugin->urids.patch_property       = symap_map(lv2_plugin->symap, LV2_PATCH__property);
  lv2_plugin->urids.patch_value          = symap_map(lv2_plugin->symap, LV2_PATCH__value);
  lv2_plugin->urids.time_Position        = symap_map(lv2_plugin->symap, LV2_TIME__Position);
  lv2_plugin->urids.time_bar             = symap_map(lv2_plugin->symap, LV2_TIME__bar);
  lv2_plugin->urids.time_barBeat         = symap_map(lv2_plugin->symap, LV2_TIME__barBeat);
  lv2_plugin->urids.time_beatUnit        = symap_map(lv2_plugin->symap, LV2_TIME__beatUnit);
  lv2_plugin->urids.time_beatsPerBar     = symap_map(lv2_plugin->symap, LV2_TIME__beatsPerBar);
  lv2_plugin->urids.time_beatsPerMinute  = symap_map(lv2_plugin->symap, LV2_TIME__beatsPerMinute);
  lv2_plugin->urids.time_frame           = symap_map(lv2_plugin->symap, LV2_TIME__frame);
  lv2_plugin->urids.time_speed           = symap_map(lv2_plugin->symap, LV2_TIME__speed);
  lv2_plugin->urids.ui_updateRate        = symap_map(lv2_plugin->symap, LV2_UI__updateRate);

#ifdef _WIN32
  lv2_plugin->temp_dir = lv2_strdup("lv2_plugin->XXXXX");
  _mktemp(lv2_plugin->temp_dir);
#else
  char* templ = lv2_strdup("/tmp/lv2_plugin->XXXXXX");
  lv2_plugin->temp_dir = lv2_strjoin(mkdtemp(templ), "/");
  free(templ);
#endif

  LV2_State_Make_Path make_path = { lv2_plugin, lv2_make_path };
  lv2_plugin->make_path_feature.data = &make_path;

  LV2_Worker_Schedule sched = { &lv2_plugin->worker, lv2_worker_schedule };
  lv2_plugin->sched_feature.data = &sched;

  LV2_Worker_Schedule ssched = { &lv2_plugin->state_worker, lv2_worker_schedule };
  lv2_plugin->state_sched_feature.data = &ssched;

  LV2_Log_Log llog = { &lv2_plugin, lv2_printf, lv2_vprintf };
  lv2_plugin->log_feature.data = &llog;

  zix_sem_init(&lv2_plugin->exit_sem, 0);
  lv2_plugin->done = &lv2_plugin->exit_sem;

  zix_sem_init(&lv2_plugin->worker.sem, 0);



  /* Load preset, if specified */
  if (preset_uri)
    {
      LilvNode* preset = lilv_new_uri (LILV_WORLD,
                                      preset_uri);

      lv2_load_presets(lv2_plugin, NULL, NULL);
      lv2_plugin->state = lilv_state_new_from_world (LILV_WORLD,
                                         &lv2_plugin->map,
                                         preset);
      lv2_plugin->preset = lv2_plugin->state;
      lilv_node_free(preset);
      if (!lv2_plugin->state)
        {
          g_warning ("Failed to find preset <%s>\n",
                  preset_uri);
          return -1;
        }
  }
  else if (lv2_plugin->state_file)
    {
      char * state_file_path =
        g_build_filename (PROJECT->states_dir,
                          lv2_plugin->state_file,
                          NULL);
      lv2_plugin->state = lilv_state_new_from_file (LILV_WORLD,
                                                    &lv2_plugin->map,
                                                    NULL,
                                                    state_file_path);
      if (!lv2_plugin->state)
        {
          g_error ("Failed to load state from %s\n", state_file_path);
        }

      LilvNode * lv2_uri =
        lilv_node_duplicate(lilv_state_get_plugin_uri(lv2_plugin->state));

      if (!lv2_uri)
        {
          g_error ("Missing plugin URI, try lv2ls to list plugins\n");
        }

      /* Find plugin */
      g_message ("Plugin:       %s\n", lilv_node_as_string(lv2_uri));
      lv2_plugin->lilv_plugin = lilv_plugins_get_by_uri(LV2_SETTINGS.lilv_plugins,
                                               lv2_uri);
      lilv_node_free(lv2_uri);
      if (!lv2_plugin->lilv_plugin)
        {
          g_error("Failed to find plugin\n");
        }

      /* Set default values for all ports */
      if (lv2_set_ports (lv2_plugin) < 0)
        {
          return -1;
        }

      lv2_plugin->control_in = (uint32_t)-1;
    }
  else
    {
      /* Set default values for all ports */
      if (lv2_create_ports (lv2_plugin) < 0)
        {
          return -1;
        }

      /* Set the zrythm plugin ports */
      for (uint32_t i = 0; i < lv2_plugin->num_ports; ++i)
        {
          LV2_Port * lv2_port = &lv2_plugin->ports[i];
          Port * port = lv2_port->port;
          port->owner_pl = plugin;
          /*if (port->type == TYPE_AUDIO)*/
            /*{*/
              if (port->flow == FLOW_INPUT)
                {
                  plugin->in_ports[plugin->num_in_ports++] = port;
                }
              else if (port->flow == FLOW_OUTPUT)
                {
                  plugin->out_ports[plugin->num_out_ports++] = port;
                }
            /*}*/
        }

      lv2_plugin->control_in = (uint32_t)-1;
    }

  /* Check that any required features are supported */
  LilvNodes* req_feats = lilv_plugin_get_required_features(lv2_plugin->lilv_plugin);
  LILV_FOREACH(nodes, f, req_feats)
    {
      const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
      if (!_feature_is_supported(lv2_plugin, uri))
        {
          g_warning ("Feature %s is not supported\n", uri);
          lilv_nodes_free(req_feats);
          return -1;
        }
    }
  lilv_nodes_free(req_feats);

  /* Check for thread-safe state restore() method. */
  LilvNode* state_threadSafeRestore = lilv_new_uri(
          LILV_WORLD, LV2_STATE__threadSafeRestore);
  if (lilv_plugin_has_feature (lv2_plugin->lilv_plugin,
                               state_threadSafeRestore))
    {
      lv2_plugin->safe_restore = true;
    }
  lilv_node_free(state_threadSafeRestore);

  if (!lv2_plugin->state)
    {
      /* Not restoring state, load the lv2_plugin->as a preset to get default */
      lv2_plugin->state = lilv_state_new_from_world(
              LILV_WORLD, &lv2_plugin->map,
              lilv_plugin_get_uri(lv2_plugin->lilv_plugin));
    }

  /* Get a lv2_plugin->UI */
  g_message (
    "Looking for native UI");
  const char* native_ui_type_uri = lv2_native_ui_type (lv2_plugin);
  lv2_plugin->uis = lilv_plugin_get_uis (lv2_plugin->lilv_plugin);
  if (!LV2_SETTINGS.opts.generic_ui && native_ui_type_uri)
    {
      const LilvNode* native_ui_type = lilv_new_uri(LILV_WORLD,
                                                    native_ui_type_uri);
      LILV_FOREACH(uis, u, lv2_plugin->uis)
        {
          const LilvUI* this_ui = lilv_uis_get(lv2_plugin->uis, u);
          const LilvNodes* types = lilv_ui_get_classes(this_ui);
          LILV_FOREACH(nodes, t, types)
            {
              const char * pt = lilv_node_as_uri (
                              lilv_nodes_get(types, t));
              g_message ("Found UI: %s", pt);
            }
          if (lilv_ui_is_supported(this_ui,
                                   suil_ui_supported,
                                   native_ui_type,
                                   &lv2_plugin->ui_type))
            {
              /* TODO: Multiple UI support */
              g_message ("UI is supported");
              lv2_plugin->ui = this_ui;
              break;
            }
          else
            {
              g_message ("UI unsupported by suil");
            }
        }
    }
  else if (!LV2_SETTINGS.opts.generic_ui && LV2_SETTINGS.opts.show_ui)
    {
      lv2_plugin->ui = lilv_uis_get(lv2_plugin->uis, lilv_uis_begin(lv2_plugin->uis));
    }

  if (!lv2_plugin->ui)
    {
      g_message (
        "Native UI not found, looking for external");
      LILV_FOREACH(uis, u, lv2_plugin->uis)
        {
          const LilvUI* ui = lilv_uis_get(lv2_plugin->uis, u);
          const LilvNodes* types = lilv_ui_get_classes(ui);
          LILV_FOREACH(nodes, t, types)
            {
              const char * pt = lilv_node_as_uri (
                              lilv_nodes_get(types, t));
              if (!strcmp (pt, "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget"))
                {
                  g_message ("Found UI: %s", pt);
                  g_message ("External KX UI selected");
                  lv2_plugin->externalui = true;
                  lv2_plugin->ui = ui;
                  lv2_plugin->ui_type = lv2_plugin->nodes.ui_externalkx;
                }
              else if (!strcmp (pt, "http://lv2plug.in/ns/extensions/ui#external"))
                {
                  g_message ("Found UI: %s", pt);
                  g_message ("External UI selected");
                  lv2_plugin->externalui = true;
                  lv2_plugin->ui_type = lv2_plugin->nodes.ui_externallv;
                  lv2_plugin->ui = ui;
                }
            }
        }
    }

  /* Create ringbuffers for UI if necessary */
  if (lv2_plugin->ui)
    {
      g_message ("Selected UI:           %s",
              lilv_node_as_uri(lilv_ui_get_uri(lv2_plugin->ui)));
    }
  else
    {
      g_message ("Selected UI:           None");
    }

  /* Create port and control structures */
  lv2_create_controls(lv2_plugin, true);
  lv2_create_controls(lv2_plugin, false);

  /* FIXME */
  /*if (!(lv2_plugin->backend = lv2_backend_init(lv2_plugin))) {*/
          /*g_error("Failed to connect to audio system");*/
  /*}*/

  if (LV2_SETTINGS.opts.buffer_size == 0) {
          /* The UI ring is fed by lv2_plugin->output ports (usually one), and the UI
             updates roughly once per cycle.  The ring size is a few times the
             size of the MIDI output to give the UI a chance to keep up.  The UI
             should be able to keep up with 4 cycles, and tests show this works
             for me, but this value might need increasing to avoid overflows.
          */
          LV2_SETTINGS.opts.buffer_size = AUDIO_ENGINE->midi_buf_size * N_BUFFER_CYCLES;
  }

  if (LV2_SETTINGS.opts.update_rate == 0.0)
    {
      /* Calculate a reasonable UI update frequency. */
      lv2_plugin->ui_update_hz = (float)AUDIO_ENGINE->sample_rate /
        AUDIO_ENGINE->midi_buf_size * 2.0f;
      lv2_plugin->ui_update_hz = MAX(25.0f, lv2_plugin->ui_update_hz);
    }
  else
    {
      /* Use user-specified UI update rate. */
      lv2_plugin->ui_update_hz = LV2_SETTINGS.opts.update_rate;
      lv2_plugin->ui_update_hz = MAX(1.0f, lv2_plugin->ui_update_hz);
    }

  /* The UI can only go so fast, clamp to reasonable limits */
  lv2_plugin->ui_update_hz     = MIN(60, lv2_plugin->ui_update_hz);
  LV2_SETTINGS.opts.buffer_size = MAX(4096, LV2_SETTINGS.opts.buffer_size);
  g_message ("Comm buffers: %d bytes", LV2_SETTINGS.opts.buffer_size);
  g_message ("Update rate:  %.01f Hz", lv2_plugin->ui_update_hz);

  /* Build options array to pass to plugin */
  const LV2_Options_Option options[] = {
          { LV2_OPTIONS_INSTANCE,
            0,
            lv2_plugin->urids.param_sampleRate,
            sizeof(float),
            lv2_plugin->urids.atom_Float,
            &AUDIO_ENGINE->sample_rate },
          { LV2_OPTIONS_INSTANCE, 0, lv2_plugin->urids.bufsz_minBlockLength,
            sizeof(int32_t), lv2_plugin->urids.atom_Int, &AUDIO_ENGINE->block_length },
          { LV2_OPTIONS_INSTANCE, 0, lv2_plugin->urids.bufsz_maxBlockLength,
            sizeof(int32_t), lv2_plugin->urids.atom_Int, &AUDIO_ENGINE->block_length },
          { LV2_OPTIONS_INSTANCE, 0, lv2_plugin->urids.bufsz_sequenceSize,
            sizeof(int32_t), lv2_plugin->urids.atom_Int, &AUDIO_ENGINE->midi_buf_size },
          { LV2_OPTIONS_INSTANCE, 0, lv2_plugin->urids.ui_updateRate,
            sizeof(float), lv2_plugin->urids.atom_Float, &lv2_plugin->ui_update_hz },
          { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
  };

  lv2_plugin->options_feature.data = (void*)&options;

  /* Create Plugin <=> UI communication buffers */
  lv2_plugin->ui_events     = zix_ring_new(LV2_SETTINGS.opts.buffer_size);
  lv2_plugin->plugin_events = zix_ring_new(LV2_SETTINGS.opts.buffer_size);
  zix_ring_mlock(lv2_plugin->ui_events);
  zix_ring_mlock(lv2_plugin->plugin_events);

  /* Instantiate the lv2_plugin->*/
  lv2_plugin->instance = lilv_plugin_instantiate(
          lv2_plugin->lilv_plugin, AUDIO_ENGINE->sample_rate,
          lv2_plugin->features);
  if (!lv2_plugin->instance)
    {
      g_warning("Failed to instantiate lv2_plugin->\n");
      return -1;
    }
  g_message ("Lilv plugin instantiated");

  lv2_plugin->ext_data.data_access =
    lilv_instance_get_descriptor(lv2_plugin->instance)->extension_data;

  /*if (!AUDIO_ENGINE->buf_size_set) {*/
          lv2_allocate_port_buffers(lv2_plugin);
  /*}*/

  /* Create workers if necessary */
  if (lilv_plugin_has_extension_data(lv2_plugin->lilv_plugin, lv2_plugin->nodes.work_interface)) {
          const LV2_Worker_Interface* iface = (const LV2_Worker_Interface*)
                  lilv_instance_get_extension_data(lv2_plugin->instance, LV2_WORKER__interface);

          lv2_worker_init(lv2_plugin, &lv2_plugin->worker, iface, true);
          if (lv2_plugin->safe_restore) {
                  lv2_worker_init(lv2_plugin, &lv2_plugin->state_worker, iface, false);
          }
  }

  /* Apply loaded state to lv2_plugin->instance if necessary */
  if (lv2_plugin->state) {
      lv2_apply_state(lv2_plugin, lv2_plugin->state);
  }

  if (LV2_SETTINGS.opts.controls) {
          for (char** c = LV2_SETTINGS.opts.controls; *c; ++c) {
                  _apply_control_arg(lv2_plugin, *c);
          }
  }

  /* Set Jack callbacks */
  /*lv2_backend_init(lv2_plugin);*/

  /* Create Jack ports and connect lv2_plugin->ports to buffers */
  for (uint32_t i = 0; i < lv2_plugin->num_ports; ++i) {
      lv2_backend_activate_port(lv2_plugin, i);
  }

  /* Print initial control values */
  if (print_controls)
    for (size_t i = 0; i < lv2_plugin->controls.n_controls; ++i)
      {
        Lv2ControlID* control = lv2_plugin->controls.controls[i];
        if (control->type == PORT)
          {// && control->value_type == lv2_plugin->>forge.Float) {
                LV2_Port* port = &lv2_plugin->ports[control->index];
                _print_control_value(lv2_plugin, port, port->control);
          }
      }

  /* Activate lv2_plugin->*/
  g_message ("Activating instance...");
  lilv_instance_activate(lv2_plugin->instance);
  g_message ("Instance activated");

  /* Discover UI */
  lv2_plugin->has_ui = lv2_discover_ui(lv2_plugin);

  /* Run UI (or prompt at console) */
  g_message ("Opening UI...");
  lv2_open_ui(lv2_plugin);
  g_message ("UI opened");

  return 0;
}

void
lv2_plugin_process (Lv2Plugin * lv2_plugin)
{
  int nframes = AUDIO_ENGINE->nframes;
  jack_client_t * client = AUDIO_ENGINE->client;

    /* If transport state is not as expected, then something has changed */
    const bool xport_changed = (
      lv2_plugin->rolling != (TRANSPORT->play_state == PLAYSTATE_ROLLING) ||
      lv2_plugin->pos.frames != PLAYHEAD.frames ||
      lv2_plugin->bpm != TRANSPORT->bpm);

    uint8_t   pos_buf[256];
    LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;
    if (xport_changed)
      {
        /* Build an LV2 position object to report change to plugin */
        lv2_atom_forge_set_buffer(&lv2_plugin->forge, pos_buf, sizeof(pos_buf));
        LV2_Atom_Forge*      forge = &lv2_plugin->forge;
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_object(forge, &frame, 0, lv2_plugin->urids.time_Position);
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_frame);
        lv2_atom_forge_long(forge, PLAYHEAD.frames);
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_speed);
        lv2_atom_forge_float(forge, TRANSPORT->play_state == PLAYSTATE_ROLLING ? 1.0 : 0.0);
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_barBeat);
        lv2_atom_forge_float(
                forge, (float) PLAYHEAD.beats - 1 + ((float) PLAYHEAD.ticks / (float) TICKS_PER_BEAT));
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_bar);
        lv2_atom_forge_long(forge, PLAYHEAD.bars - 1);
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_beatUnit);
        lv2_atom_forge_int(
          forge,
          transport_get_beat_unit (TRANSPORT));
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_beatsPerBar);
        lv2_atom_forge_float(forge, TRANSPORT->beats_per_bar);
        lv2_atom_forge_key(forge, lv2_plugin->urids.time_beatsPerMinute);
        lv2_atom_forge_float(forge, TRANSPORT->bpm);

        if (1)
          {
            /*char* str = sratom_to_turtle(*/
                    /*lv2_plugin->sratom, &lv2_plugin->unmap, "time:", NULL, NULL,*/
                    /*lv2_pos->type, lv2_pos->size,*/
                    /*LV2_ATOM_BODY(lv2_pos));*/
            /*lv2_ansi_start(stdout, 36);*/
            /*printf("\n## Position ##\n%s\n", str);*/
            /*lv2_ansi_reset(stdout);*/
            /*free(str);*/
          }
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
  for (uint32_t p = 0; p < lv2_plugin->num_ports; ++p)
    {
      LV2_Port * lv2_port = &lv2_plugin->ports[p];
      Port * port = lv2_port->port;
      if (port->type == TYPE_AUDIO)
        {
          /* Connect lv2 ports  to plugin port buffers */
          /*port->nframes = nframes;*/
          lilv_instance_connect_port(
                  lv2_plugin->instance, p,
                  port->buf);
#ifdef HAVE_JACK_METADATA
      }
    else if (port->type == TYPE_CV && lv2_port->sys_port)
      {
              /* Connect plugin port directly to Jack port buffer */
              lilv_instance_connect_port(
                      lv2_plugin->instance, p,
                      jack_port_get_buffer(lv2_port->sys_port, nframes));
#endif
      }
    else if (port->type == TYPE_EVENT && port->flow == FLOW_INPUT) {
              lv2_evbuf_reset(lv2_port->evbuf, true);

              /* Write transport change event if applicable */
              LV2_Evbuf_Iterator iter = lv2_evbuf_begin(lv2_port->evbuf);
              if (xport_changed)
                {
                  lv2_evbuf_write(&iter, 0, 0,
                                  lv2_pos->type, lv2_pos->size,
                                  (const uint8_t*)LV2_ATOM_BODY(lv2_pos));
                }

              if (lv2_plugin->request_update)
                {
                  /* Plugin state has changed, request an update */
                  const LV2_Atom_Object get = {
                          { sizeof(LV2_Atom_Object_Body),
                            lv2_plugin->urids.atom_Object },
                          { 0, lv2_plugin->urids.patch_Get } };
                  lv2_evbuf_write(&iter, 0, 0,
                                  get.atom.type, get.atom.size,
                                  (const uint8_t*)LV2_ATOM_BODY(&get));
                }

              if (port->midi_events.num_events > 0)
                {
                  /* Write Jack MIDI input */
                  for (uint32_t i = 0; i < port->midi_events.num_events; ++i)
                    {
                      jack_midi_event_t * ev = &port->midi_events.jack_midi_events[i];
                      lv2_evbuf_write(&iter,
                                      ev->time, 0,
                                      lv2_plugin->urids.midi_MidiEvent,
                                      ev->size, ev->buffer);
                    }
                }
              midi_events_clear (&port->midi_events);
      } else if (port->type == TYPE_EVENT) {
              /* Clear event output for plugin to write to */
              lv2_evbuf_reset(lv2_port->evbuf, false);
      }
    }
  lv2_plugin->request_update = false;

  /* Run plugin for this cycle */
  const bool send_ui_updates = lv2_run (lv2_plugin, nframes);

  /* Deliver MIDI output and UI events */
  for (uint32_t p = 0; p < lv2_plugin->num_ports; ++p)
    {
      LV2_Port* const lv2_port = &lv2_plugin->ports[p];
      Port * port = lv2_port->port;
      if (port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL &&
            lilv_port_has_property(lv2_plugin->lilv_plugin,
                                   lv2_port->lilv_port,
                                   lv2_plugin->nodes.lv2_reportsLatency))
        {
          if (lv2_plugin->plugin_latency != lv2_port->control)
            {
              lv2_plugin->plugin_latency = lv2_port->control;
              jack_recompute_total_latencies(client);
            }
        }
      else if (port->flow == FLOW_OUTPUT && port->type == TYPE_EVENT)
          {
            void* buf = NULL;

            for (LV2_Evbuf_Iterator i = lv2_evbuf_begin(lv2_port->evbuf);
                 lv2_evbuf_is_valid(i);
                 i = lv2_evbuf_next(i)) {
                    // Get event from LV2 buffer
                    uint32_t frames, subframes, type, size;
                    uint8_t* body;
                    lv2_evbuf_get(i, &frames, &subframes, &type, &size, &body);

                    if (buf && type == lv2_plugin->urids.midi_MidiEvent) {
                        /* Write MIDI event to port TODO */
                        /*jack_midi_event_write(buf, frames, body, size);*/
                    }

                    if (lv2_plugin->has_ui && !lv2_port->old_api) {
                      // Forward event to UI
                      lv2_send_to_ui(lv2_plugin, p, type, size, body);
                    }
            }
          }
        else if (send_ui_updates &&
                 port->type == TYPE_CONTROL)
          {
            char buf[sizeof(Lv2ControlChange) +
              sizeof(float)];
            Lv2ControlChange* ev = (Lv2ControlChange*)buf;
            ev->index    = p;
            ev->protocol = 0;
            ev->size     = sizeof(float);
            *(float*)ev->body = lv2_port->control;
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
  for (uint32_t i = 0; i < lv2_plugin->num_ports; ++i) {
          if (lv2_plugin->ports[i].evbuf) {
                  lv2_evbuf_free(lv2_plugin->ports[i].evbuf);
          }
  }

  /* Deactivate lv2_plugin->*/
  suil_instance_free(lv2_plugin->ui_instance);
  lilv_instance_deactivate(lv2_plugin->instance);
  lilv_instance_free(lv2_plugin->instance);

  /* Clean up */
  free(lv2_plugin->ports);
  zix_ring_free(lv2_plugin->ui_events);
  zix_ring_free(lv2_plugin->plugin_events);
  for (LilvNode** n = (LilvNode**)&lv2_plugin->nodes; *n; ++n) {
          lilv_node_free(*n);
  }
  symap_free(lv2_plugin->symap);
  zix_sem_destroy(&lv2_plugin->symap_lock);
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
lv2_save_state (Lv2Plugin * lv2_plugin, const char * dir)
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

  if (state)
    {
      char * label = g_strdup_printf ("%s.ttl",
                                      lv2_plugin->plugin->descr->name);
      g_message ("before save state %d",
                 lv2_plugin->symap->size);
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
        }
      char * tmp = g_path_get_basename (dir);
      lv2_plugin->state_file =
        g_build_filename (tmp,
                          label,
                          NULL);
      g_free (label);
      g_free (tmp);
      lilv_state_free (state);

      return rc;
    }
  else
    {
      g_warning ("Could create state");
    }

  return -1;
}
