/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/engine.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/carla/native_plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

static uint32_t
host_get_buffer_size (
  NativeHostHandle handle)
{
  uint32_t buffer_size = 512;
  if (AUDIO_ENGINE->block_length > 0)
    buffer_size =
      (uint32_t) AUDIO_ENGINE->block_length;

  return buffer_size;
}

static double
host_get_sample_rate (
  NativeHostHandle handle)
{
  double sample_rate = 44000.0;
  if (AUDIO_ENGINE->sample_rate > 0)
    sample_rate = (double) AUDIO_ENGINE->sample_rate;

  return sample_rate;
}

static bool
host_is_offline (
  NativeHostHandle handle)
{
  return !AUDIO_ENGINE->run;
}

static const NativeTimeInfo*
host_get_time_info (
  NativeHostHandle handle)
{
  CarlaNativePlugin * plugin =
    (CarlaNativePlugin *) handle;
  return &plugin->time_info;
}

static bool
host_write_midi_event (
  NativeHostHandle        handle,
  const NativeMidiEvent * event)
{
  return 0;
}

static void
host_ui_parameter_changed (
  NativeHostHandle handle,
  uint32_t index,
  float value)
{
  g_message ("handle ui param changed");
}

static void
host_ui_custom_data_changed (
  NativeHostHandle handle,
  const char* key,
  const char* value)
{
}

static void
host_ui_closed (
  NativeHostHandle handle)
{
  g_message ("ui closed");
}

static intptr_t
host_dispatcher (
  NativeHostHandle handle,
  NativeHostDispatcherOpcode opcode,
  int32_t index,
  intptr_t value,
  void* ptr,
  float opt)
{
  /* TODO */
  g_message ("host dispatcher");

  return 0;
}

static CarlaNativePlugin *
create (CarlaPluginType type)
{
  CarlaNativePlugin * self =
    calloc (1, sizeof (CarlaNativePlugin));

  self->host.handle = ZRYTHM;
  self->host.uiName =
    g_strdup ("Zrythm - Carla");
  self->host.uiParentId = 0;
    /*(uintptr_t)*/
    /*z_gtk_widget_get_gdk_window_id (*/
      /*GTK_WIDGET (MAIN_WINDOW));*/

  /* set resources dir */
  const char * carla_filename =
    carla_get_library_filename ();
  char * tmp = io_get_dir (carla_filename);
  char * dir = io_get_dir (tmp);
  g_free (tmp);
  tmp = io_get_dir (dir);
  g_free (dir);
  dir = tmp;
  self->host.resourceDir =
    g_build_filename (
      dir, "share", "carla", "resources", NULL);
  g_free (dir);

  self->host.get_buffer_size =
    host_get_buffer_size;
  self->host.get_sample_rate =
    host_get_sample_rate;
  self->host.is_offline =
    host_is_offline;
  self->host.get_time_info =
    host_get_time_info;
  self->host.write_midi_event =
    host_write_midi_event;
  self->host.ui_parameter_changed =
    host_ui_parameter_changed;
  self->host.ui_custom_data_changed =
    host_ui_custom_data_changed;
  self->host.ui_closed =
    host_ui_closed;
  self->host.ui_open_file = NULL;
  self->host.ui_save_file = NULL;
  self->host.dispatcher = host_dispatcher;

  self->time_info.bbt.valid = 1;

  switch (type)
    {
    case CARLA_PLUGIN_RACK:
      self->descriptor =
        carla_get_native_rack_plugin ();
      break;
    case CARLA_PLUGIN_PATCHBAY:
      self->descriptor =
        carla_get_native_patchbay_plugin ();
      break;
    default:
      g_warning ("Carla plugin type not supported");
      break;
    }
  self->handle =
    self->descriptor->instantiate (
      &self->host);

  return self;
}

/**
 * Returns a filled in descriptor for the given
 * type.
 *
 * @param ins Set the descriptor to be an instrument.
 */
PluginDescriptor *
carla_native_plugin_get_descriptor (
  CarlaPluginType type,
  int             ins)
{
  CarlaNativePlugin * plugin =
    create (type);
  const NativePluginDescriptor * descr =
    plugin->descriptor;

  PluginDescriptor * self =
    calloc (1, sizeof (PluginDescriptor));

  self->author =
    g_strdup (descr->maker);
  self->name =
    g_strdup_printf (
      "Zrythm %s: %s",
      ins ? "Instrument" : "Effect",
      descr->name);
  if (ins)
    self->category = PC_INSTRUMENT;
  else
    self->category = PC_UTILITY;
  self->category_str =
    g_strdup (_("Plugin Adapter"));
  self->num_audio_ins = (int) descr->audioIns;
  self->num_audio_outs = (int) descr->audioOuts;
  self->num_midi_ins = (int) descr->midiIns;
  self->num_midi_outs = (int) descr->midiOuts;
  self->num_ctrl_ins = (int) descr->paramIns;
  self->num_ctrl_outs = (int) descr->paramOuts;
  self->protocol = PROT_CARLA;
  self->carla_type = type;

  carla_native_plugin_free (plugin);

  return self;
}

/**
 * Creates an instance of a CarlaNativePlugin inside
 * the given Plugin.
 *
 * The given Plugin must have its descriptor filled in.
 */
void
carla_native_plugin_create (
  Plugin * plugin)
{
  CarlaNativePlugin * self =
    create (plugin->descr->carla_type);

  plugin->carla = self;
  self->plugin = plugin;
}

static void
create_ports (
  CarlaNativePlugin * self)
{
  Port * port;

  /* create midi in/out */
  port =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "MIDI in");
  plugin_add_in_port (
    self->plugin, port);
  port =
    port_new_with_type (
      TYPE_EVENT, FLOW_OUTPUT, "MIDI out");
  plugin_add_out_port (
    self->plugin, port);

  /* create stereo in/out */
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "Stereo in L");
  plugin_add_in_port (
    self->plugin, port);
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "Stereo in R");
  plugin_add_in_port (
    self->plugin, port);
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Stereo out L");
  plugin_add_out_port (
    self->plugin, port);
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Stereo out R");
  plugin_add_out_port (
    self->plugin, port);

  /* create controls */
  uint32_t num_ports =
    carla_native_plugin_get_param_count (self);
  for (uint32_t i = 0; i < num_ports; i++)
    {
      const NativeParameter * param =
        carla_native_plugin_get_param_info (
          self, i);
      PortFlow flow;
      flow =
        param->hints & NATIVE_PARAMETER_IS_OUTPUT ?
        FLOW_OUTPUT : FLOW_INPUT;
      port =
        port_new_with_type (
          TYPE_CONTROL, flow, param->name);
      port->carla_param_index = i;
      switch (flow)
        {
        case FLOW_INPUT:
          plugin_add_in_port (
            self->plugin, port);
          break;
        case FLOW_OUTPUT:
          plugin_add_out_port (
            self->plugin, port);
          break;
        default:
          g_warn_if_reached ();
        }
    }
}

/**
 * Instantiates the plugin.
 *
 * @ret 0 if no errors, non-zero if errors.
 */
int
carla_native_plugin_instantiate (
  CarlaNativePlugin * self)
{
  g_return_val_if_fail (
    self->handle &&
    self->descriptor->activate &&
    self->descriptor->ui_show,
    -1);

  /* create ports */
  create_ports (self);

  g_message ("activating carla plugin...");
  self->descriptor->activate (self->handle);
  g_message ("carla plugin activated");

  return 0;
}

/**
 * Shows or hides the UI.
 */
void
carla_native_plugin_show_ui (
  CarlaNativePlugin * self,
  int                 show)
{
  self->descriptor->ui_show (
    self->handle, 1);
}

/**
 * Returns the plugin Port corresponding to the
 * given parameter.
 */
Port *
carla_native_plugin_get_port_from_param (
  CarlaNativePlugin *     self,
  const NativeParameter * param)
{
  Plugin * pl = self->plugin;
  Port * port;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];
      if (port->identifier.type != TYPE_CONTROL)
        continue;

      if (param ==
            carla_native_plugin_get_param_info (
              self,
              port->carla_param_index))
        return port;
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];
      if (port->identifier.type != TYPE_CONTROL)
        continue;

      if (param ==
            carla_native_plugin_get_param_info (
              self,
              port->carla_param_index))
        return port;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Deactivates, cleanups and frees the instance.
 */
void
carla_native_plugin_free (
  CarlaNativePlugin * self)
{
  self->descriptor->deactivate (self->handle);
  self->descriptor->cleanup (self->handle);

  free (self);
}
