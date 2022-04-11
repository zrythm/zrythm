/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <lv2/instance-access/instance-access.h>

static const bool debug = true;

/**
 * Returns if the UI of the plugin is resizable.
 */
bool
lv2_ui_is_resizable (Lv2Plugin * plugin)
{
  g_return_val_if_fail (
    plugin->plugin->setting->ui_uri, false);

  LilvNode * s = lilv_new_uri (
    LILV_WORLD, plugin->plugin->setting->ui_uri);
  LilvNode * p = lilv_new_uri (
    LILV_WORLD, LV2_CORE__optionalFeature);
  LilvNode * fs =
    lilv_new_uri (LILV_WORLD, LV2_UI__fixedSize);
  LilvNode * nrs = lilv_new_uri (
    LILV_WORLD, LV2_UI__noUserResize);

  LilvNodes * fs_matches =
    lilv_world_find_nodes (LILV_WORLD, s, p, fs);
  LilvNodes * nrs_matches =
    lilv_world_find_nodes (LILV_WORLD, s, p, nrs);

  lilv_nodes_free (nrs_matches);
  lilv_nodes_free (fs_matches);
  lilv_node_free (nrs);
  lilv_node_free (fs);
  lilv_node_free (p);
  lilv_node_free (s);

  return !fs_matches && !nrs_matches;
}

/**
 * Suil callback to get the index of the Lv2Port
 * corresponding to the given symbol.
 */
static uint32_t
get_port_index (
  SuilController controller,
  const char *   symbol)
{
  Lv2Plugin * const plugin =
    (Lv2Plugin *) controller;
  Port * port = plugin_get_port_by_symbol (
    plugin->plugin, symbol);

  return port && port->lilv_port_index >= 0
           ? (uint32_t) port->lilv_port_index
           : LV2UI_INVALID_PORT_INDEX;
}

/**
 * Read and apply control change events from UI,
 * for plugins that have their own UIs.
 *
 * Called in the real-time audio thread during
 * plugin processing.
 *
 * @param nframes Used for event ports.
 */
REALTIME
void
lv2_ui_read_and_apply_events (
  Lv2Plugin * plugin,
  uint32_t    nframes)
{
  /*if (!lv2_plugin_has_custom_ui (plugin))*/
  /*{*/
  /*return;*/
  /*}*/

  Lv2ControlChange ev;
  const size_t     space = zix_ring_read_space (
        plugin->ui_to_plugin_events);
  for (size_t i = 0; i < space;
       i += sizeof (ev) + ev.size)
    {
      zix_ring_read (
        plugin->ui_to_plugin_events, (char *) &ev,
        sizeof (ev));
      char body[ev.size];
      if (
        zix_ring_read (
          plugin->ui_to_plugin_events, body, ev.size)
        != ev.size)
        {
          g_critical (
            "Error reading from UI ring buffer");
          break;
        }
      g_return_if_fail (
        (int) ev.index >= 0
        && (int) ev.index
             < plugin->plugin->num_lilv_ports);
      Port * port =
        plugin->plugin->lilv_ports[ev.index];

      bool have_custom_ui =
        !plugin->plugin->setting->force_generic_ui;

      /* float control change - this is only for
       * plugins with custom UIs */
      if (ev.protocol == 0 && have_custom_ui)
        {
          assert (ev.size == sizeof (float));
          g_return_if_fail (port);
          port_set_control_value (
            port, *(float *) body, 0, 0);
          port->received_ui_event = 1;

#if 0
          /* note: should not be printing in the
           * realtime thread */
          g_debug (
            "plugin %s float control '%s' change: "
            "%f - has custom UI %d",
            plugin->plugin->setting->descr->name,
            port->id.label,
            (double) * (float *) body,
            have_custom_ui);
#endif
        }
      else if (
        ev.protocol == PM_URIDS.atom_eventTransfer)
        {
          LV2_Evbuf_Iterator e =
            lv2_evbuf_end (port->evbuf);
          const LV2_Atom * const atom =
            (const LV2_Atom *) body;
          lv2_evbuf_write (
            &e, nframes, 0, atom->type, atom->size,
            (const uint8_t *) LV2_ATOM_BODY_CONST (
              atom));

#if 0
          /* note: should not be printing in the
           * realtime thread */
          g_debug (
            "%s: plugin %s event transfer "
            "event for %s - has custom UI %d",
            __func__, plugin->plugin->setting->descr->name,
            port->id.label,
            have_custom_ui);
#endif
        }
      else
        {
          g_critical (
            "Unknown control change protocol %d",
            ev.protocol);
        }
    }
}

/**
 * Similar to lv2_ui_send_event_from_plugin_to_ui
 * except that it passes a float instead of an
 * LV2 atom.
 *
 * @param lv2_port The port to pass the value of.
 */
void
lv2_ui_send_control_val_event_from_plugin_to_ui (
  Lv2Plugin * lv2_plugin,
  Port *      port)
{
  if (
    !lv2_plugin->plugin->visible
    || lv2_plugin->plugin->instantiation_failed)
    return;

#if 0
  g_debug ("%s: %s: %s (%d)",
    __func__,
    lv2_plugin->plugin->setting->descr->name,
    port->id.sym,
    port->lilv_port_index);
#endif

  char
    buf[sizeof (Lv2ControlChange) + sizeof (float)];
  Lv2ControlChange * ev = (Lv2ControlChange *) buf;
  ev->index = (uint32_t) port->lilv_port_index;
  ev->protocol = 0;
  ev->size = sizeof (float);
  *(float *) ev->body = port->control;
  port->automating = 0;

  if (
    zix_ring_write (
      lv2_plugin->plugin_to_ui_events, buf,
      sizeof (buf))
    < sizeof (buf))
    {
      PluginDescriptor * descr =
        lv2_plugin->plugin->setting->descr;
      g_warning (
        "Buffer overflow when writing "
        "events from plugin %s (%s) to "
        "its UI",
        descr->name, descr->uri);
    }

  port->last_sent_control = port->control;
}

/**
 * Send event to UI, called during the real time
 * audio thread when processing the plugin.
 *
 * @param type Atom type.
 */
REALTIME
int
lv2_ui_send_event_from_plugin_to_ui (
  Lv2Plugin *  plugin,
  uint32_t     port_index,
  uint32_t     type,
  uint32_t     size,
  const void * body)
{
  /* TODO: Be more disciminate about what to send */
  char evbuf
    [sizeof (Lv2ControlChange) + sizeof (LV2_Atom)];
  Lv2ControlChange * ev =
    (Lv2ControlChange *) evbuf;
  ev->index = port_index;
  ev->protocol = PM_URIDS.atom_eventTransfer;
  ev->size = (uint32_t) sizeof (LV2_Atom) + size;

  LV2_Atom * atom = (LV2_Atom *) ev->body;
  atom->type = type;
  atom->size = (uint32_t) size;

  if (
    zix_ring_write_space (
      plugin->plugin_to_ui_events)
    >= sizeof (evbuf) + size)
    {
      zix_ring_write (
        plugin->plugin_to_ui_events, evbuf,
        sizeof (evbuf));
      zix_ring_write (
        plugin->plugin_to_ui_events,
        (const char *) body, size);
      return 1;
    }
  else
    {
      PluginDescriptor * descr =
        plugin->plugin->setting->descr;
      g_warning (
        "Buffer overflow when sending plugin %s "
        "(%s) event to its UI",
        descr->name, descr->uri);
      return 0;
    }
}

/**
 * Write events from the plugin's UI to the plugin.
 *
 * @param protocol Format, 0 for control-port event,
 *   > 0 for message.
 */
void
lv2_ui_send_event_from_ui_to_plugin (
  Lv2Plugin *  plugin,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     protocol,
  const void * buffer)
{
  if (
    (int) port_index < 0
    || (int) port_index
         >= plugin->plugin->num_lilv_ports)
    {
      g_warning (
        "UI write to out of range port index %d",
        port_index);
      return;
    }

#if 0
  Port * port =
    plugin->plugin->lilv_ports[port_index];
  if (port->lilv_port_index > -1)
    {
      g_debug ("port %d", port_index);
    }
  else
    {
      g_debug (
        "param %d (%s)", port_index, port->id.sym);
    }
#endif

  if (
    protocol != 0
    && protocol != PM_URIDS.atom_eventTransfer)
    {
      g_warning (
        "UI write with unsupported protocol %d (%s)",
        protocol,
        lv2_urid_unmap_uri (plugin, protocol));
      return;
    }

  /* also see https://git.open-music-kontrollers.ch/lv2/sherlock.lv2/tree/atom_inspector_nk.c#n39 */
  if (debug && protocol == PM_URIDS.atom_eventTransfer)
    {
      const LV2_Atom * atom =
        (const LV2_Atom *) buffer;
      g_debug (
        "[atom] type <%s>, %d bytes",
        plugin->unmap.unmap (
          plugin->unmap.handle, atom->type),
        atom->size);
      char * str = sratom_to_turtle (
        plugin->sratom, &plugin->unmap,
        "plugin:", NULL, NULL, atom->type,
        atom->size, LV2_ATOM_BODY_CONST (atom));
      g_message (
        "## UI => Plugin (%u bytes) ##\n%s",
        atom->size, str);
      free (str);
    }

  char buf[sizeof (Lv2ControlChange) + buffer_size];
  Lv2ControlChange * ev = (Lv2ControlChange *) buf;
  ev->index = port_index;
  ev->protocol = protocol;
  ev->size = buffer_size;
  memcpy (ev->body, buffer, buffer_size);
  zix_ring_write (
    plugin->ui_to_plugin_events, buf,
    (uint32_t) sizeof (buf));
}

/**
 * Instantiates the plugin UI.
 */
void
lv2_ui_instantiate (Lv2Plugin * plugin)
{
  plugin->suil_host = suil_host_new (
    (SuilPortWriteFunc)
      lv2_ui_send_event_from_ui_to_plugin,
    get_port_index, NULL, NULL);
  plugin->external_ui_widget = NULL;

  char * bundle_uri = lv2_plugin_get_ui_bundle_uri (
    plugin->plugin->setting->descr->uri,
    plugin->plugin->setting->ui_uri);
  g_return_if_fail (bundle_uri);
  char * binary_uri = lv2_plugin_get_ui_binary_uri (
    plugin->plugin->setting->descr->uri,
    plugin->plugin->setting->ui_uri);
  char * bundle_path =
    lilv_file_uri_parse (bundle_uri, NULL);
  char * binary_path =
    lilv_file_uri_parse (binary_uri, NULL);

  /*
   * Data access feature.
   *
   * Pass the LV2_Extension_Data_Feature initialized
   * during plugin instantiation. This allows the UI
   * to call the data_access method in ext_data to
   * receive data from the plugin instance.
   */
  const LV2_Feature data_feature = {
    LV2_DATA_ACCESS_URI,
    &plugin->ext_data_feature,
  };

  const LV2_Feature idle_feature = {
    LV2_UI__idleInterface, NULL
  };

  /**
   * Instance access feature.
   *
   * Pass the plugin instance handle.
   */
  const LV2_Feature instance_feature = {
    LV2_INSTANCE_ACCESS_URI,
    lilv_instance_get_handle (plugin->instance)
  };

  /*
   * Instantiate plugin UI.
   *
   * Note: suil appends and implements the following
   * features:
   * * LV2_UI__portMap
   * * LV2_UI__portSubscribe
   * * LV2_UI__touch
   * * LV2_UI__parent
   * * LV2_UI__resize
   * * LV2_UI__idleInterface
   */
  if (plugin->has_external_ui)
    {
      /* deprecated URI for backwards
       * compatibility */
      const LV2_Feature external_ui_feature = {
        .URI = LV2_EXTERNAL_UI_DEPRECATED_URI,
        .data = &plugin->extui,
      };

      const LV2_Feature external_kxui_feature = {
        .URI = LV2_EXTERNAL_UI__Host,
        .data = &plugin->extui,
      };

      const LV2_Feature * ui_features[] = {
        &plugin->map_feature,
        &plugin->unmap_feature,
        &instance_feature,
        &data_feature,
        &idle_feature,
        &plugin->log_feature,
        &external_ui_feature,
        &external_kxui_feature,
        &plugin->options_feature,
        NULL
      };

      char * ui_class = lv2_plugin_get_ui_class (
        plugin->plugin->setting->descr->uri,
        plugin->plugin->setting->ui_uri);
      plugin->suil_instance = suil_instance_new (
        plugin->suil_host, plugin, ui_class,
        plugin->plugin->setting->descr->uri,
        plugin->plugin->setting->ui_uri, ui_class,
        bundle_path, binary_path, ui_features);
      g_free (ui_class);

      if (plugin->suil_instance)
        {
          plugin->external_ui_widget =
            suil_instance_get_widget (
              plugin->suil_instance);
        }
      else
        {
          plugin->has_external_ui = 0;
        }
    }
  else
    {
      const LV2_Feature * ui_features[] = {
        &plugin->map_feature,
        &plugin->unmap_feature,
        &instance_feature,
        &data_feature,
        &idle_feature,
        &plugin->log_feature,
        &plugin->options_feature,
        NULL,
      };

      char * ui_class = lv2_plugin_get_ui_class (
        plugin->plugin->setting->descr->uri,
        plugin->plugin->setting->ui_uri);
      plugin->suil_instance = suil_instance_new (
        plugin->suil_host, plugin, LV2_UI__Gtk4UI,
        plugin->plugin->setting->descr->uri,
        plugin->plugin->setting->ui_uri, ui_class,
        bundle_path, binary_path, ui_features);
      g_free (ui_class);
    }

  lilv_free (binary_path);
  lilv_free (bundle_path);
  g_free (binary_uri);
  g_free (bundle_uri);

  if (!plugin->suil_instance)
    {
      g_warning (
        "Failed to get UI instance for %s",
        plugin->plugin->setting->descr->name);
    }
}

/**
 * Inits the LV2 plugin UI.
 *
 * To be called for generic, suil-wrapped and
 * external UIs.
 */
void
lv2_ui_init (Lv2Plugin * plugin)
{
  // Set initial control port values
  for (int i = 0;
       i < plugin->plugin->num_lilv_ports; ++i)
    {
      Port * port = plugin->plugin->lilv_ports[i];
      if (
        port->id.type == TYPE_CONTROL
        && !(port->id.flags & PORT_FLAG_IS_PROPERTY))
        {
          lv2_gtk_ui_port_event (
            plugin, (uint32_t) i, sizeof (float),
            0, &port->control);
        }
    }

  if (plugin->control_in != -1)
    {
      /* Send patch:Get message for initial
       * parameters/etc */
      LV2_Atom_Forge * forge = &plugin->main_forge;
      LV2_Atom_Forge_Frame frame;
      uint8_t              buf[1024];
      lv2_atom_forge_set_buffer (
        forge, buf, sizeof (buf));
      lv2_atom_forge_object (
        forge, &frame, 0, PM_URIDS.patch_Get);

      const LV2_Atom * atom =
        lv2_atom_forge_deref (forge, frame.ref);
      lv2_ui_send_event_from_ui_to_plugin (
        plugin, (uint32_t) plugin->control_in,
        lv2_atom_total_size (atom),
        PM_URIDS.atom_eventTransfer, atom);
      lv2_atom_forge_pop (forge, &frame);
    }
}
