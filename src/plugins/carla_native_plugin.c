/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#ifdef HAVE_CARLA

#include <stdlib.h>

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/carla/carla_engine_interface.h"
#include "plugins/carla/carla_plugin_interface.h"
#include "plugins/carla_native_plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/string.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Tick callback for the plugin UI.
 */
static int
carla_plugin_tick_cb (
  GtkWidget * widget,
  GdkFrameClock * frame_clock,
  CarlaNativePlugin * self)
{
  if (self->plugin->visible &&
      MAIN_WINDOW)
    {
      CarlaPluginHandle plugin =
        carla_native_plugin_get_plugin_handle (
          self);
      carla_plugin_ui_idle (plugin);

      return G_SOURCE_CONTINUE;
    }
  else
    return G_SOURCE_REMOVE;
}

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
    sample_rate =
      (double) AUDIO_ENGINE->sample_rate;

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

/**
 * Carla engine callback.
 */
static void
engine_callback (
  void* ptr,
  EngineCallbackOpcode action,
  unsigned int pluginId,
  int value1,
  int value2,
  float value3,
  const char* valueStr)
{
  CarlaNativePlugin * self =
    (CarlaNativePlugin *) ptr;

  switch (action)
    {
    case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
      break;
    case ENGINE_CALLBACK_PLUGIN_ADDED:
      self->carla_plugin_id = pluginId;
      /*g_message ("Carla: plugin added");*/
      break;
    case ENGINE_CALLBACK_UI_STATE_CHANGED:
      if (value1 == 1)
        {
          self->plugin->visible = value1;
          g_message ("plugin ui visible");
        }
      else if (value1 == 0)
        {
          self->plugin->visible = value1;
        }
      else
        {
          g_warning (
            "Plugin \"%s\" UI crashed",
            self->plugin->descr->name);
        }
      break;
    case ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
    case ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
    case ENGINE_CALLBACK_IDLE:
    case ENGINE_CALLBACK_ENGINE_STOPPED:
      /* ignore */
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

static CarlaNativePlugin *
_create ()
{
  CarlaNativePlugin * self =
    calloc (1, sizeof (CarlaNativePlugin));

  self->host.handle = self;
  self->host.uiName =
    g_strdup ("Zrythm");
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

  return self;
}

#if 0
static CarlaNativePlugin *
create_carla_internal (CarlaPluginType type)
{
  CarlaNativePlugin * self =
    _create ();

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
#endif

static CarlaNativePlugin *
create_plugin (
  const PluginDescriptor * descr,
  PluginType   type)
{
  CarlaNativePlugin * self =
    _create ();

  /* instantiate the plugin to get its info */
  self->descriptor =
    carla_get_native_rack_plugin ();
  self->handle =
    self->descriptor->instantiate (
      &self->host);
  CarlaEngineHandle engine =
    carla_engine_get_from_native_plugin (
      self->descriptor, self->handle);
  carla_engine_set_callback (
    engine, engine_callback, self);
  self->carla_plugin_id = 0;
  int ret = 0;
  switch (descr->protocol)
    {
    case PROT_LV2:
      g_message ("lv2");
      ret =
        carla_engine_add_plugin_simple (
          engine, type,
          descr->name, descr->name, descr->uri,
          0, NULL);
      break;
    case PROT_VST:
      g_message ("vst");
      ret =
        carla_engine_add_plugin_simple (
          engine, type,
          descr->path, descr->name, descr->name,
          0, NULL);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
  g_return_val_if_fail (ret == 1, NULL);

  return self;
}

/**
 * Wrapper to get the CarlaPlugin instance.
 */
CarlaPluginHandle
carla_native_plugin_get_plugin_handle (
  CarlaNativePlugin * self)
{
  CarlaEngineHandle engine =
    carla_engine_get_from_native_plugin (
      self->descriptor, self->handle);
  CarlaPluginHandle plugin =
    carla_engine_get_plugin (
      engine, self->carla_plugin_id);

  return plugin;
}

/**
 * Processes the plugin for this cycle.
 */
void
carla_native_plugin_proces (
  CarlaNativePlugin * self,
  const long          g_start_frames,
  const nframes_t     nframes)
{

  self->time_info.playing =
    TRANSPORT_IS_ROLLING;
  self->time_info.frame =
    (uint64_t) g_start_frames;
  self->time_info.bbt.bar =
    PLAYHEAD->bars;
  self->time_info.bbt.beat =
    PLAYHEAD->beats;
  self->time_info.bbt.tick =
    PLAYHEAD->sixteenths *
      TICKS_PER_SIXTEENTH_NOTE +
    PLAYHEAD->ticks;
  Position bar_start;
  position_set_to_bar (
    &bar_start, PLAYHEAD->bars);
  self->time_info.bbt.barStartTick =
    (double)
    (PLAYHEAD->total_ticks -
     bar_start.total_ticks);
  self->time_info.bbt.beatsPerBar =
    (float)
    TRANSPORT->beats_per_bar;
  self->time_info.bbt.beatType =
    (float)
    TRANSPORT->beat_unit;
  self->time_info.bbt.ticksPerBeat =
    TRANSPORT->ticks_per_beat;
  self->time_info.bbt.beatsPerMinute =
    TRANSPORT->bpm;

  switch (self->plugin->descr->protocol)
    {
    case PROT_VST:
    {
      const float * inbuf[] =
        {
          self->stereo_in->l->buf,
          self->stereo_in->r->buf
        };
      float * outbuf[] =
        {
          self->stereo_out->l->buf,
          self->stereo_out->r->buf
        };
      const float * cvinbuf[] =
        {
          self->cv_in->l->buf,
          self->cv_in->r->buf
        };
      float * cvoutbuf[] =
        {
          self->cv_out->l->buf,
          self->cv_out->r->buf
        };
      CarlaPluginHandle plugin =
        carla_native_plugin_get_plugin_handle (self);
      g_warn_if_fail (plugin);
      carla_plugin_process (
        plugin, inbuf,
        outbuf, cvinbuf, cvoutbuf, nframes);
    }
      break;
#if 0
    case PROT_SFZ:
    {
      float * inbuf[] =
        {
          self->stereo_in->l->buf,
          self->stereo_in->r->buf
        };
      float * outbuf[] =
        {
          self->stereo_out->l->buf,
          self->stereo_out->r->buf
        };
      for (int i = 0;
           i < self->midi_in->midi_events->
            num_events; i++)
        {
          MidiEvent * ev =
            &self->midi_in->midi_events->events[i];
          self->midi_events[i].time =
            ev->time;
          self->midi_events[i].size = 3;
          self->midi_events[i].data[0] =
            ev->raw_buffer[0];
          self->midi_events[i].data[1] =
            ev->raw_buffer[1];
          self->midi_events[i].data[2] =
            ev->raw_buffer[2];
        }
      self->num_midi_events =
        (uint32_t)
        self->midi_in->midi_events->num_events;
      if (self->num_midi_events > 0)
        {
          g_message (
            "Carla plugin %s has %d MIDI events",
            self->plugin->descr->name,
            self->num_midi_events);
        }
      self->descriptor->process (
        self->handle, inbuf, outbuf, nframes,
        self->midi_events, self->num_midi_events);
    }
      break;
#endif
    default:
      break;
    }
}

/**
 * Returns a filled in descriptor for the given
 * type.
 *
 * @param ins Set the descriptor to be an instrument.
 *
 * FIXME delete
 */
/*PluginDescriptor **/
/*carla_native_plugin_get_descriptor (*/
  /*CarlaPluginType type,*/
  /*int             ins)*/
/*{*/
  /*CarlaNativePlugin * plugin =*/
    /*create (type);*/
  /*const NativePluginDescriptor * descr =*/
    /*plugin->descriptor;*/

  /*PluginDescriptor * self =*/
    /*calloc (1, sizeof (PluginDescriptor));*/

  /*self->author =*/
    /*g_strdup (descr->maker);*/
  /*self->name =*/
    /*g_strdup_printf (*/
      /*"Zrythm %s: %s",*/
      /*ins ? "Instrument" : "Effect",*/
      /*descr->name);*/
  /*if (ins)*/
    /*self->category = PC_INSTRUMENT;*/
  /*else*/
    /*self->category = PC_UTILITY;*/
  /*self->category_str =*/
    /*g_strdup (_("Plugin Adapter"));*/
  /*self->num_audio_ins = (int) descr->audioIns;*/
  /*self->num_audio_outs = (int) descr->audioOuts;*/
  /*self->num_midi_ins = (int) descr->midiIns;*/
  /*self->num_midi_outs = (int) descr->midiOuts;*/
  /*self->num_ctrl_ins = (int) descr->paramIns;*/
  /*self->num_ctrl_outs = (int) descr->paramOuts;*/
  /*self->protocol = PROT_CARLA;*/
  /*self->carla_type = type;*/

  /*carla_native_plugin_free (plugin);*/

  /*return self;*/
/*}*/

static PluginCategory
carla_category_to_zrythm_category (
  int category)
{
  switch (category)
    {
    case CARLA_PLUGIN_CATEGORY_NONE:
      return PLUGIN_CATEGORY_NONE;
      break;
    case CARLA_PLUGIN_CATEGORY_SYNTH:
      return PC_INSTRUMENT;
      break;
    case CARLA_PLUGIN_CATEGORY_DELAY:
      return PC_DELAY;
      break;
    case CARLA_PLUGIN_CATEGORY_EQ:
      return PC_EQ;
      break;
    case CARLA_PLUGIN_CATEGORY_FILTER:
      return PC_FILTER;
      break;
    case CARLA_PLUGIN_CATEGORY_DISTORTION:
      return PC_DISTORTION;
      break;
    case CARLA_PLUGIN_CATEGORY_DYNAMICS:
      return PC_DYNAMICS;
      break;
    case CARLA_PLUGIN_CATEGORY_MODULATOR:
      return PC_MODULATOR;
      break;
    case CARLA_PLUGIN_CATEGORY_UTILITY:
      return PC_UTILITY;
      break;
    case CARLA_PLUGIN_CATEGORY_OTHER:
      return PLUGIN_CATEGORY_NONE;
      break;
    }
  g_return_val_if_reached (PLUGIN_CATEGORY_NONE);
}

static char *
carla_category_to_zrythm_category_str (
  int category)
{
  switch (category)
    {
    case CARLA_PLUGIN_CATEGORY_NONE:
      return g_strdup ("Plugin");
      break;
    case CARLA_PLUGIN_CATEGORY_SYNTH:
      return g_strdup ("Instrument");
      break;
    case CARLA_PLUGIN_CATEGORY_DELAY:
      return g_strdup ("Delay");
      break;
    case CARLA_PLUGIN_CATEGORY_EQ:
      return g_strdup ("Equalizer");
      break;
    case CARLA_PLUGIN_CATEGORY_FILTER:
      return g_strdup ("Filter");
      break;
    case CARLA_PLUGIN_CATEGORY_DISTORTION:
      return g_strdup ("Distortion");
      break;
    case CARLA_PLUGIN_CATEGORY_DYNAMICS:
      return g_strdup ("Dynamics");
      break;
    case CARLA_PLUGIN_CATEGORY_MODULATOR:
      return g_strdup ("Modulator");
      break;
    case CARLA_PLUGIN_CATEGORY_UTILITY:
      return g_strdup ("Utility");
      break;
    case CARLA_PLUGIN_CATEGORY_OTHER:
      return g_strdup ("Plugin");
      break;
    }
  g_return_val_if_reached (NULL);
}

/**
 * Returns a filled in descriptor from the
 * given binary path.
 */
PluginDescriptor *
carla_native_plugin_get_descriptor_from_path (
  const char * path,
  PluginType   type)
{
  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));

  descr->path =
    g_strdup (path);

  descr->open_with_carla = 1;
  switch (type)
    {
#if 0
    case PLUGIN_INTERNAL:
      descr->protocol = PROT_CARLA_INTERNAL;
      break;
#endif
    case PLUGIN_LADSPA:
      descr->protocol = PROT_LADSPA;
      break;
    case PLUGIN_DSSI:
      descr->protocol = PROT_DSSI;
      break;
    case PLUGIN_LV2:
      descr->protocol = PROT_LV2;
      break;
    case PLUGIN_VST2:
      descr->protocol = PROT_VST;
      break;
#if 0
    case PLUGIN_SF2:
      descr->protocol = PROT_SF2;
      break;
    case PLUGIN_SFZ:
      descr->protocol = PROT_SFZ;
      break;
    case PLUGIN_JACK:
      descr->protocol = PROT_JACK;
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }

  if (descr->protocol == PROT_VST)
    {
      char * carla_discovery_native =
        g_build_filename (
          carla_get_library_folder (),
          "carla-discovery-native",
          NULL);

      /* read plugin info */
      FILE *fp;
      char line[1035];

      /* Open the command for reading. */
      char * command =
        g_strdup_printf (
          "%s vst %s",
          carla_discovery_native,
          path);
      fp = popen (command, "r");
      g_free (command);
      if (fp == NULL)
        g_return_val_if_reached (NULL);

      /* Read the output a line at a time -
       * output it. */
      while (fgets (
                line, sizeof (line) - 1, fp) !=
               NULL)
        {
          if (g_str_has_prefix (
                line,
                "carla-discovery::name::"))
            {
              descr->name =
                g_strdup (
                  g_strchomp (&line[23]));
            }
          else if (
            g_str_has_prefix (
              line, "carla-discovery::maker::"))
            {
              descr->author =
                g_strdup (
                  g_strchomp (&line[24]));
            }
          else if (
            g_str_has_prefix (
              line,
              "carla-discovery::audio.ins::"))
            {
              descr->num_audio_ins =
                atoi (g_strchomp (&line[28]));
            }
          else if (
            g_str_has_prefix (
              line,
              "carla-discovery::audio.outs::"))
            {
              descr->num_audio_outs =
                atoi (g_strchomp (&line[29]));
            }
          else if (
            g_str_has_prefix (
              line,
              "carla-discovery::midi.ins::"))
            {
              descr->num_midi_ins =
                atoi (g_strchomp (&line[27]));
            }
          else if (
            g_str_has_prefix (
              line,
              "carla-discovery::midi.outs::"))
            {
              descr->num_midi_outs =
                atoi (g_strchomp (&line[28]));
            }
          else if (
            g_str_has_prefix (
              line,
              "carla-discovery::parameters.ins::"))
            {
              descr->num_ctrl_ins =
                atoi (g_strchomp (&line[33]));
            }
        }

      /* close */
      pclose (fp);

      descr->category = PLUGIN_CATEGORY_NONE;
      descr->category_str =
        g_strdup ("Plugin");

      if (!descr->name)
        {
          plugin_descriptor_free (descr);
          return NULL;
        }
    }

  return descr;
}

/**
 * Returns a filled in descriptor from the
 * CarlaCachedPluginInfo.
 */
PluginDescriptor *
carla_native_plugin_get_descriptor_from_cached (
  const CarlaCachedPluginInfo * info,
  PluginType              type)
{
  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));

  descr->open_with_carla = 1;
  switch (type)
    {
#if 0
    case PLUGIN_INTERNAL:
      descr->protocol = PROT_CARLA_INTERNAL;
      break;
    case PLUGIN_LADSPA:
      descr->protocol = PROT_LADSPA;
      break;
    case PLUGIN_DSSI:
      descr->protocol = PROT_DSSI;
      break;
#endif
    case PLUGIN_LV2:
      descr->protocol = PROT_LV2;
      break;
    case PLUGIN_VST2:
      descr->protocol = PROT_VST;
      break;
#if 0
    case PLUGIN_SF2:
      descr->protocol = PROT_SF2;
      break;
    case PLUGIN_SFZ:
      descr->protocol = PROT_SFZ;
      break;
    case PLUGIN_JACK:
      descr->protocol = PROT_JACK;
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }
  descr->name =
    g_strdup (info->name);
  descr->author =
    g_strdup (info->maker);
  descr->num_audio_ins = (int) info->audioIns;
  descr->num_audio_outs = (int) info->audioOuts;
  descr->num_midi_ins = (int) info->midiIns;
  descr->num_midi_outs = (int) info->midiOuts;
  descr->num_ctrl_ins = (int) info->parameterIns;
  descr->num_ctrl_outs = (int) info->parameterOuts;

  descr->category =
    carla_category_to_zrythm_category (
      info->category);
  descr->category_str =
    carla_category_to_zrythm_category_str (
      info->category);

  return descr;
}

static CarlaNativePlugin *
create_from_descr (
  PluginDescriptor * descr)
{
#if 0
  g_return_val_if_fail (
    descr->open_with_carla ||
    (descr->protocol == PROT_CARLA_INTERNAL &&
       descr->carla_type > CARLA_PLUGIN_NONE),
    NULL);
#endif
  g_return_val_if_fail (
    descr->open_with_carla, NULL);

  CarlaNativePlugin * self = NULL;
  switch (descr->protocol)
    {
    case PROT_LV2:
      self =
        create_plugin (descr, PLUGIN_LV2);
      break;
    case PROT_VST:
      self =
        create_plugin (descr, PLUGIN_VST2);
      break;
#if 0
    case PROT_SFZ:
      self =
        create_plugin (descr, PLUGIN_SFZ);
      break;
    case PROT_CARLA_INTERNAL:
      create_carla_internal (
        descr->carla_type);
      break;
#endif
    default:
      g_warn_if_reached ();
      break;
    }
  g_warn_if_fail (self);

  return self;
}

/**
 * Creates an instance of a CarlaNativePlugin inside
 * the given Plugin.
 *
 * The given Plugin must have its descriptor filled in.
 */
void
carla_native_plugin_new_from_descriptor (
  Plugin * plugin)
{
  CarlaNativePlugin * self =
    create_from_descr (plugin->descr);

  plugin->carla = self;
  self->plugin = plugin;
}

static void
create_ports (
  CarlaNativePlugin * self)
{
  Port * port;
  Port * tmp;

  /* create midi in/out */
  port =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "MIDI in");
  plugin_add_in_port (
    self->plugin, port);
  self->midi_in = port;
  port =
    port_new_with_type (
      TYPE_EVENT, FLOW_OUTPUT, "MIDI out");
  plugin_add_out_port (
    self->plugin, port);
  self->midi_out = port;

  /* create stereo in/out */
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "Stereo in L");
  plugin_add_in_port (
    self->plugin, port);
  tmp = port;
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "Stereo in R");
  plugin_add_in_port (
    self->plugin, port);
  self->stereo_in =
    stereo_ports_new_from_existing (
      tmp, port);

  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "CV in L");
  plugin_add_in_port (
    self->plugin, port);
  tmp = port;
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_INPUT, "CV in R");
  plugin_add_in_port (
    self->plugin, port);
  self->cv_in =
    stereo_ports_new_from_existing (
      tmp, port);

  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Stereo out L");
  plugin_add_out_port (
    self->plugin, port);
  tmp = port;
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "Stereo out R");
  plugin_add_out_port (
    self->plugin, port);
  self->stereo_out =
    stereo_ports_new_from_existing (
      tmp, port);

  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "CV out L");
  plugin_add_out_port (
    self->plugin, port);
  tmp = port;
  port =
    port_new_with_type (
      TYPE_AUDIO, FLOW_OUTPUT, "CV out R");
  plugin_add_out_port (
    self->plugin, port);
  self->cv_out =
    stereo_ports_new_from_existing (
      tmp, port);

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
      port->carla_param_id = i;
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

  /* get internal engine and add plugin */
  CarlaEngineHandle engine =
    carla_engine_get_from_native_plugin (
      self->descriptor, self->handle);
  carla_engine_set_callback (
    engine, engine_callback, self);
  carla_engine_add_plugin_simple (
    engine, PLUGIN_VST2,
    self->plugin->descr->path,
    self->plugin->descr->name,
    self->plugin->descr->name,
    0, NULL);

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
carla_native_plugin_open_ui (
  CarlaNativePlugin * self,
  int                 show)
{
  switch (self->plugin->descr->protocol)
    {
    case PROT_VST:
    case PROT_LV2:
      {
        CarlaPluginHandle plugin =
          carla_native_plugin_get_plugin_handle (
            self);
        g_warn_if_fail (plugin);
        carla_plugin_show_custom_ui (
          plugin, show);
        self->plugin->visible = 1;

        g_warn_if_fail (MAIN_WINDOW);
        gtk_widget_add_tick_callback (
          GTK_WIDGET (MAIN_WINDOW),
          (GtkTickCallback) carla_plugin_tick_cb,
          self, NULL);
      }
      break;
#if 0
    case PROT_SFZ:
      break;
#endif
    default: break;
    }
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
              self, port->carla_param_id))
        return port;
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];
      if (port->identifier.type != TYPE_CONTROL)
        continue;

      if (param ==
            carla_native_plugin_get_param_info (
              self, port->carla_param_id))
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
#endif
