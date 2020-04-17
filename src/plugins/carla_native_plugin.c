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
#include "plugins/carla_native_plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/file.h"
#include "utils/io.h"
#include "utils/string.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <CarlaHost.h>

void
carla_native_plugin_init_loaded (
  CarlaNativePlugin * self)
{
  carla_native_plugin_new_from_descriptor (
    self->plugin);

  char * state_dir =
    io_path_get_parent_dir (
      self->state_file);
  carla_native_plugin_load_state (
    self->plugin->carla, state_dir);
  g_free (state_dir);

  carla_native_plugin_free (self);
}

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
      self->native_plugin_descriptor->ui_idle (
        self->native_plugin_handle);

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
  g_message ("write midi event");
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
_create ()
{
  CarlaNativePlugin * self =
    calloc (1, sizeof (CarlaNativePlugin));

  self->native_host_descriptor.handle = self;
  self->native_host_descriptor.uiName =
    g_strdup ("Zrythm");
  self->native_host_descriptor.uiParentId = 0;
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
  self->native_host_descriptor.resourceDir =
    g_build_filename (
      dir, "share", "carla", "resources", NULL);
  g_free (dir);

  self->native_host_descriptor.get_buffer_size =
    host_get_buffer_size;
  self->native_host_descriptor.get_sample_rate =
    host_get_sample_rate;
  self->native_host_descriptor.is_offline =
    host_is_offline;
  self->native_host_descriptor.get_time_info =
    host_get_time_info;
  self->native_host_descriptor.write_midi_event =
    host_write_midi_event;
  self->native_host_descriptor.ui_parameter_changed =
    host_ui_parameter_changed;
  self->native_host_descriptor.ui_custom_data_changed =
    host_ui_custom_data_changed;
  self->native_host_descriptor.ui_closed =
    host_ui_closed;
  self->native_host_descriptor.ui_open_file = NULL;
  self->native_host_descriptor.ui_save_file = NULL;
  self->native_host_descriptor.dispatcher = host_dispatcher;

  self->time_info.bbt.valid = 1;

  return self;
}

static CarlaNativePlugin *
create_plugin (
  const PluginDescriptor * descr,
  PluginType   type)
{
  CarlaNativePlugin * self = _create ();

  /* instantiate the plugin to get its info */
  self->native_plugin_descriptor =
    carla_get_native_rack_plugin ();
  self->native_plugin_handle =
    self->native_plugin_descriptor->instantiate (
      &self->native_host_descriptor);
  self->host_handle =
    carla_create_native_plugin_host_handle (
      self->native_plugin_descriptor,
      self->native_plugin_handle);
  self->carla_plugin_id = 0;

  /* set binary paths */
  carla_set_engine_option (
    self->host_handle,
    ENGINE_OPTION_PATH_BINARIES, 0,
    CONFIGURE_BINDIR);

  int ret = 0;
  switch (descr->protocol)
    {
    case PROT_LV2:
      ret =
        carla_add_plugin (
          self->host_handle,
          descr->arch == ARCH_64 ?
            BINARY_NATIVE : BINARY_WIN32,
          type, NULL, descr->name,
          descr->uri, 0, NULL, 0);
      break;
    case PROT_VST:
      ret =
        carla_add_plugin (
          self->host_handle,
          descr->arch == ARCH_64 ?
            BINARY_NATIVE : BINARY_WIN32,
          type, descr->path, descr->name,
          descr->name, 0, NULL, 0);
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  if (ret != 1)
    {
      g_warning (
        "Error adding carla plugin: %s",
        carla_get_last_error (self->host_handle));
      return NULL;
    }

  /* enable various messages */
#define ENABLE_OPTION(x) \
  carla_set_option ( \
    self->host_handle, 0, \
    PLUGIN_OPTION_##x, true)

  ENABLE_OPTION (FORCE_STEREO);
  ENABLE_OPTION (SEND_CONTROL_CHANGES);
  ENABLE_OPTION (SEND_CHANNEL_PRESSURE);
  ENABLE_OPTION (SEND_NOTE_AFTERTOUCH);
  ENABLE_OPTION (SEND_PITCHBEND);
  ENABLE_OPTION (SEND_ALL_SOUND_OFF);
  ENABLE_OPTION (SEND_PROGRAM_CHANGES);

  return self;
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

  PluginDescriptor * descr =
    self->plugin->descr;
  switch (descr->protocol)
    {
    case PROT_LV2:
    case PROT_VST:
    {
      float * inbuf[2];
      float dummy_inbuf1[nframes];
      memset (
        dummy_inbuf1, 0, nframes * sizeof (float));
      float dummy_inbuf2[nframes];
      memset (
        dummy_inbuf2, 0, nframes * sizeof (float));
      float * outbuf[2];
      float dummy_outbuf1[nframes];
      memset (
        dummy_outbuf1, 0, nframes * sizeof (float));
      float dummy_outbuf2[nframes];
      memset (
        dummy_outbuf2, 0, nframes * sizeof (float));
      int i = 0;
      int audio_ports = 0;
      while (i < self->plugin->num_in_ports)
        {
          Port * port = self->plugin->in_ports[i];
          if (port->id.type == TYPE_AUDIO)
            {
              inbuf[audio_ports++] =
                self->plugin->in_ports[i]->buf;
            }
          if (audio_ports == 2)
            break;
          i++;
        }
      switch (audio_ports)
        {
        case 0:
          inbuf[0] = dummy_inbuf1;
          inbuf[1] = dummy_inbuf2;
          break;
        case 1:
          inbuf[1] = dummy_inbuf2;
          break;
        default:
          break;
        }

      i = 0;
      audio_ports = 0;
      while (i < self->plugin->num_out_ports)
        {
          Port * port = self->plugin->out_ports[i];
          if (port->id.type == TYPE_AUDIO)
            {
              outbuf[audio_ports++] =
                self->plugin->out_ports[i]->buf;
            }
          if (audio_ports == 2)
            break;
          i++;
        }
      switch (audio_ports)
        {
        case 0:
          outbuf[0] = dummy_outbuf1;
          outbuf[1] = dummy_outbuf2;
          break;
        case 1:
          outbuf[1] = dummy_outbuf2;
          break;
        default:
          break;
        }

      /* get main midi port */
      Port * port = NULL;
      for (i = 0;
           i < self->plugin->num_in_ports; i++)
        {
          port = self->plugin->in_ports[i];
          if (port->id.type == TYPE_EVENT)
            {
              break;
            }
          else
            port = NULL;
        }

      int num_events =
        port ? port->midi_events->num_events : 0;
      NativeMidiEvent events[4000];
      for (i = 0; i < num_events; i++)
        {
          MidiEvent * ev =
            &port->midi_events->events[i];
          events[i].time = ev->time;
          events[i].size = 3;
          events[i].data[0] = ev->raw_buffer[0];
          events[i].data[1] = ev->raw_buffer[1];
          events[i].data[2] = ev->raw_buffer[2];
          /*midi_event_print (ev);*/
        }
      if (num_events > 0)
        {
          g_message (
            "Carla plugin %s has %d MIDI events",
            self->plugin->descr->name,
            num_events);
        }

      /*g_warn_if_reached ();*/
      self->native_plugin_descriptor->process (
        self->native_plugin_handle, inbuf, outbuf,
        nframes, events, (uint32_t) num_events);
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

static ZPluginCategory
carla_category_to_zrythm_category (
  int category)
{
  switch (category)
    {
    case PLUGIN_CATEGORY_NONE:
      return ZPLUGIN_CATEGORY_NONE;
      break;
    case PLUGIN_CATEGORY_SYNTH:
      return PC_INSTRUMENT;
      break;
    case PLUGIN_CATEGORY_DELAY:
      return PC_DELAY;
      break;
    case PLUGIN_CATEGORY_EQ:
      return PC_EQ;
      break;
    case PLUGIN_CATEGORY_FILTER:
      return PC_FILTER;
      break;
    case PLUGIN_CATEGORY_DISTORTION:
      return PC_DISTORTION;
      break;
    case PLUGIN_CATEGORY_DYNAMICS:
      return PC_DYNAMICS;
      break;
    case PLUGIN_CATEGORY_MODULATOR:
      return PC_MODULATOR;
      break;
    case PLUGIN_CATEGORY_UTILITY:
      return PC_UTILITY;
      break;
    case PLUGIN_CATEGORY_OTHER:
      return ZPLUGIN_CATEGORY_NONE;
      break;
    }
  g_return_val_if_reached (ZPLUGIN_CATEGORY_NONE);
}

static char *
carla_category_to_zrythm_category_str (
  int category)
{
  switch (category)
    {
    case PLUGIN_CATEGORY_NONE:
      return g_strdup ("Plugin");
      break;
    case PLUGIN_CATEGORY_SYNTH:
      return g_strdup ("Instrument");
      break;
    case PLUGIN_CATEGORY_DELAY:
      return g_strdup ("Delay");
      break;
    case PLUGIN_CATEGORY_EQ:
      return g_strdup ("Equalizer");
      break;
    case PLUGIN_CATEGORY_FILTER:
      return g_strdup ("Filter");
      break;
    case PLUGIN_CATEGORY_DISTORTION:
      return g_strdup ("Distortion");
      break;
    case PLUGIN_CATEGORY_DYNAMICS:
      return g_strdup ("Dynamics");
      break;
    case PLUGIN_CATEGORY_MODULATOR:
      return g_strdup ("Modulator");
      break;
    case PLUGIN_CATEGORY_UTILITY:
      return g_strdup ("Utility");
      break;
    case PLUGIN_CATEGORY_OTHER:
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

      descr->category = ZPLUGIN_CATEGORY_NONE;
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
  CarlaNativePlugin * self,
  bool                loading)
{
  Port * port;
  char tmp[500];
  char name[4000];

  PluginDescriptor * descr = self->plugin->descr;
  if (!loading)
    {
      for (int i = 0; i < descr->num_audio_ins; i++)
        {
          strcpy (tmp, _("Audio in"));
          sprintf (name, "%s %d", tmp, i);
          port =
            port_new_with_type (
              TYPE_AUDIO, FLOW_INPUT, name);
          plugin_add_in_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_audio_outs; i++)
        {
          strcpy (tmp, _("Audio out"));
          sprintf (name, "%s %d", tmp, i);
          port =
            port_new_with_type (
              TYPE_AUDIO, FLOW_OUTPUT, name);
          plugin_add_out_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_midi_ins; i++)
        {
          strcpy (tmp, _("MIDI in"));
          sprintf (name, "%s %d", tmp, i);
          port =
            port_new_with_type (
              TYPE_EVENT, FLOW_INPUT, name);
          plugin_add_in_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_midi_outs; i++)
        {
          strcpy (tmp, _("MIDI out"));
          sprintf (name, "%s %d", tmp, i);
          port =
            port_new_with_type (
              TYPE_EVENT, FLOW_OUTPUT, name);
          plugin_add_out_port (
            self->plugin, port);
        }
    }

  /* create controls */
  const CarlaPortCountInfo * param_counts =
    carla_get_parameter_count_info (
      self->host_handle, 0);
  for (uint32_t i = 0; i < param_counts->ins; i++)
    {
      if (loading)
        {
          port =
            carla_native_plugin_get_port_from_param_id (
              self, i);
          port_set_control_value (
            port,
            carla_get_current_parameter_value (
              self->host_handle, 0, i), false,
            false);
        }
      else
        {
          const CarlaParameterInfo * param_info =
            carla_get_parameter_info (
              self->host_handle, 0, i);
          port =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT,
              param_info->name);
          port->id.flags |=
            PORT_FLAG_PLUGIN_CONTROL;
          port->carla_param_id = (int) i;
          plugin_add_in_port (
            self->plugin, port);
        }
    }
}

/**
 * Instantiates the plugin.
 *
 * @param loading Whether loading an existing plugin
 *   or not.
 * @ret 0 if no errors, non-zero if errors.
 */
int
carla_native_plugin_instantiate (
  CarlaNativePlugin * self,
  bool                loading)
{
  g_return_val_if_fail (
    self->native_plugin_handle &&
    self->native_plugin_descriptor->activate &&
    self->native_plugin_descriptor->ui_show,
    -1);

  /* create or load ports */
  if (!loading)
    create_ports (self, loading);

  g_message ("activating carla plugin...");
  self->native_plugin_descriptor->activate (
    self->native_plugin_handle);
  g_message ("carla plugin activated");

  /* create or load ports */
  if (loading)
    create_ports (self, loading);

  return 0;
}

/**
 * Shows or hides the UI.
 */
void
carla_native_plugin_open_ui (
  CarlaNativePlugin * self,
  bool                show)
{
  switch (self->plugin->descr->protocol)
    {
    case PROT_VST:
    case PROT_LV2:
      {
        carla_show_custom_ui (
          self->host_handle, 0, show);
          self->plugin->visible = show;

        if (show)
          {
            g_warn_if_fail (MAIN_WINDOW);
            gtk_widget_add_tick_callback (
              GTK_WIDGET (MAIN_WINDOW),
              (GtkTickCallback)
              carla_plugin_tick_cb,
              self, NULL);
          }

          EVENTS_PUSH (
            ET_PLUGIN_WINDOW_VISIBILITY_CHANGED,
            self->plugin);
      }
      break;
#if 0
    case PROT_SFZ:
      break;
#endif
    default: break;
    }
}

float
carla_native_plugin_get_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id)
{
  return
    carla_get_current_parameter_value (
      self->host_handle, 0, id);
}

void
carla_native_plugin_set_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id,
  float               val)
{
  carla_set_parameter_value (
    self->host_handle, 0, id, val);
}

/**
 * Returns the plugin Port corresponding to the
 * given parameter.
 */
Port *
carla_native_plugin_get_port_from_param_id (
  CarlaNativePlugin * self,
  const uint32_t      id)
{
  Plugin * pl = self->plugin;
  Port * port;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];
      if (port->id.type != TYPE_CONTROL)
        continue;

      if ((int) id == port->carla_param_id)
        return port;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Saves the state inside the given directory.
 */
int
carla_native_plugin_save_state (
  CarlaNativePlugin * self,
  char *              dir)
{
  if (self->state_file)
    {
      g_free (self->state_file);
    }
  io_mkdir (dir);
  self->state_file =
    g_build_filename (
      dir, CARLA_STATE_FILENAME, NULL);
  int ret =
    carla_save_plugin_state (
      self->host_handle, 0, self->state_file);
  g_warn_if_fail (file_exists (self->state_file));
  return !ret;
}

/**
 * Loads the state from the given directory or from
 * its state file.
 *
 * @param dir The directory to save the state from,
 *   or NULL to use
 *   \ref CarlaNativePlugin.state_file.
 */
void
carla_native_plugin_load_state (
  CarlaNativePlugin * self,
  char *              dir)
{
  char * state_file;
  if (dir)
    {
      state_file =
        g_build_filename (
          dir, CARLA_STATE_FILENAME, NULL);
    }
  else
    {
      state_file = self->state_file;
    }

  g_warn_if_fail (file_exists (state_file));
  carla_load_plugin_state (
    self->host_handle, 0, state_file);

  if (dir)
    {
      g_free (state_file);
    }
}

/**
 * Deactivates, cleanups and frees the instance.
 */
void
carla_native_plugin_free (
  CarlaNativePlugin * self)
{
  if (self->native_plugin_descriptor)
    {
      self->native_plugin_descriptor->deactivate (
        self->native_plugin_handle);
      self->native_plugin_descriptor->cleanup (
        self->native_plugin_handle);
    }

  free (self);
}
#endif
