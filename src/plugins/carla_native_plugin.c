/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#ifdef HAVE_CARLA

#include <math.h>
#include <stdlib.h>

#include "audio/engine.h"
#include "audio/midi_event.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/cached_plugin_descriptors.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/carla_native_plugin.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <CarlaHost.h>

typedef enum
{
  Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
  Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_FAILED,
} ZPluginsCarlaNativePluginError;

#define Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR \
  z_plugins_carla_native_plugin_error_quark ()
GQuark z_plugins_carla_native_plugin_error_quark (void);
G_DEFINE_QUARK (
  z-plugins-carla-native-plugin-error-quark, z_plugins_carla_native_plugin_error)

static PluginType
get_plugin_type_from_protocol (
  PluginProtocol protocol)
{
  switch (protocol)
    {
    case PROT_LV2:
      return PLUGIN_LV2;
    case PROT_AU:
      return PLUGIN_AU;
    case PROT_VST:
      return PLUGIN_VST2;
    case PROT_VST3:
      return PLUGIN_VST3;
    case PROT_SFZ:
      return PLUGIN_SFZ;
    case PROT_SF2:
      return PLUGIN_SF2;
    case PROT_DSSI:
      return PLUGIN_DSSI;
    case PROT_LADSPA:
      return PLUGIN_LADSPA;
    default:
      g_return_val_if_reached (0);
    }

  g_return_val_if_reached (0);
}

void
carla_native_plugin_init_loaded (
  CarlaNativePlugin * self)
{
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
  if (PROJECT && AUDIO_ENGINE &&
      AUDIO_ENGINE->block_length > 0)
    buffer_size =
      (uint32_t) AUDIO_ENGINE->block_length;

  return buffer_size;
}

static double
host_get_sample_rate (
  NativeHostHandle handle)
{
  double sample_rate = 44000.0;
  if (PROJECT && AUDIO_ENGINE &&
      AUDIO_ENGINE->sample_rate > 0)
    sample_rate =
      (double) AUDIO_ENGINE->sample_rate;

  return sample_rate;
}

static bool
host_is_offline (
  NativeHostHandle handle)
{
  if (!PROJECT || !AUDIO_ENGINE)
    {
      return true;
    }

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
  /*g_message ("write midi event");*/

  CarlaNativePlugin * self =
    (CarlaNativePlugin *) handle;

  Port * midi_out_port =
    carla_native_plugin_get_midi_out_port (self);
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (midi_out_port), 0);

  midi_byte_t buf[event->size];
  for (int i = 0; i < event->size; i++)
    {
      buf[i] = event->data[i];
    }
  midi_events_add_event_from_buf (
    midi_out_port->midi_events, event->time,
    buf, event->size, false);

  return 0;
}

static void
host_ui_parameter_changed (
  NativeHostHandle handle,
  uint32_t index,
  float value)
{
  g_message ("handle ui param changed");
  CarlaNativePlugin * self =
    (CarlaNativePlugin *) handle;
  Port * port =
    carla_native_plugin_get_port_from_param_id (
      self, index);
  if (!port)
    {
      g_message (
        "%s: no port found for param %u",
        __func__, index);
      return;
    }

  port_set_control_value (
    port, value, false, false);
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
  /*g_debug ("host dispatcher (opcode %d)", opcode);*/
  switch (opcode)
    {
    case NATIVE_HOST_OPCODE_HOST_IDLE:
      /*g_debug ("host idle");*/
      /* some expensive computation is happening.
       * this is used so that the GTK ui does not
       * block */
      /* note: disabled because some logic depends
       * on this plugin being activated */
#if 0
      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }
#endif
      break;
#if 0
    case NATIVE_HOST_OPCODE_INTERNAL_PLUGIN:
      /* falktx: you will need to call the new
       * juce functions, then return 1 on the
       * INTERNAL_PLUGIN host opcode.
       * when that opcode returns 1, carla-plugin
       * will let the host do the juce idling
       * which is for the best here, since you want
       * to show UIs without carla itself */
      return 1;
      break;
#endif
    case NATIVE_HOST_OPCODE_GET_FILE_PATH:
      g_debug ("get file path");
      g_return_val_if_fail (ptr, 0);
      if (string_is_equal ((char *) ptr, "carla"))
        {
          g_debug ("ptr is carla");
          return (intptr_t) PROJECT->dir;
        }
      break;
    case NATIVE_HOST_OPCODE_UI_RESIZE:
      g_debug ("ui resize");
      /* TODO handle UI resize */
      break;
    case NATIVE_HOST_OPCODE_UI_TOUCH_PARAMETER:
      g_debug ("ui touch");
      break;
    case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
      /* TODO handle UI close */
      g_debug ("UI unavailable");
      break;
    default:
      break;
    }

  return 0;
}

static void
engine_callback (
  void *               ptr,
  EngineCallbackOpcode action,
  uint                 plugin_id,
  int                  val1,
  int                  val2,
  int                  val3,
  float                valf,
  const char *         val_str)
{
  CarlaNativePlugin * self =
    (CarlaNativePlugin *) ptr;

  switch (action)
    {
    case ENGINE_CALLBACK_UI_STATE_CHANGED:
      switch (val1)
        {
        case 0:
        case -1:
          self->plugin->visible = false;
          self->plugin->visible = false;
          break;
        case 1:
          self->plugin->visible = true;
          break;
        }
      EVENTS_PUSH (
        ET_PLUGIN_VISIBILITY_CHANGED,
        self->plugin);
      break;
    case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
      /* if plugin was deactivated and we didn't
       * explicitly tell it to deactivate */
      if (val1 == PARAMETER_ACTIVE
          && val2 == 0 && val3 == 0
          && self->plugin->activated
          && !self->plugin->deactivating
          && !self->loading_state)
        {
          /* send crash signal */
          EVENTS_PUSH (
            ET_PLUGIN_CRASHED, self->plugin);
        }
      break;
    default:
      break;
    }
}

void
carla_native_plugin_populate_banks (
  CarlaNativePlugin * self)
{
  /* add default bank and preset */
  PluginBank * pl_def_bank =
    plugin_add_bank_if_not_exists (
      self->plugin,
      LV2_ZRYTHM__defaultBank,
      _("Default bank"));
  PluginPreset * pl_def_preset =
    plugin_preset_new ();
  pl_def_preset->uri =
    g_strdup (LV2_ZRYTHM__initPreset);
  pl_def_preset->name = g_strdup (_("Init"));
  plugin_add_preset_to_bank (
    self->plugin, pl_def_bank, pl_def_preset);

  /*GString * presets_gstr = g_string_new (NULL);*/

  uint32_t count =
    carla_get_program_count (
      self->host_handle, 0);
  for (uint32_t i = 0; i < count; i++)
    {
      PluginPreset * pl_preset =
        plugin_preset_new ();
      pl_preset->carla_program = (int) i;
      pl_preset->name =
        g_strdup (
          carla_get_program_name (
            self->host_handle, 0, i));
      plugin_add_preset_to_bank (
        self->plugin, pl_def_bank, pl_preset);

#if 0
      g_string_append_printf (
        presets_gstr,
        "found preset %s (%d)\n",
        pl_preset->name, pl_preset->carla_program);
#endif
    }

  g_message ("found %d presets", count);

#if 0
  char * str = g_string_free (presets_gstr, false);
  g_message ("%s", str);
  g_free (str);
#endif
}

bool
carla_native_plugin_has_custom_ui (
  const PluginDescriptor * descr)
{
#if 0
  CarlaNativePlugin * native_pl = _create (NULL);

  /* instantiate the plugin to get its info */
  native_pl->native_plugin_descriptor =
    carla_get_native_rack_plugin ();
  native_pl->native_plugin_handle =
    native_pl->native_plugin_descriptor->instantiate (
      &native_pl->native_host_descriptor);
  native_pl->host_handle =
    carla_create_native_plugin_host_handle (
      native_pl->native_plugin_descriptor,
      native_pl->native_plugin_handle);
  PluginType type =
    get_plugin_type_from_protocol (descr->protocol);
  carla_add_plugin (
    native_pl->host_handle,
    descr->arch == ARCH_64 ?
      BINARY_NATIVE : BINARY_WIN32,
    type, descr->path, descr->name,
    descr->uri, descr->unique_id, NULL, 0);
  const CarlaPluginInfo * info =
    carla_get_plugin_info (
      native_pl->host_handle, 0);
  g_return_val_if_fail (info, false);
  bool has_custom_ui =
    info->hints & PLUGIN_HAS_CUSTOM_UI;

  carla_native_plugin_free (native_pl);

  return has_custom_ui;
#endif
  g_return_val_if_reached (false);
}

/**
 * Processes the plugin for this cycle.
 */
void
carla_native_plugin_process (
  CarlaNativePlugin * self,
  const long          g_start_frames,
  const nframes_t  local_offset,
  const nframes_t     nframes)
{

  self->time_info.playing =
    TRANSPORT_IS_ROLLING;
  self->time_info.frame =
    (uint64_t) g_start_frames;
  self->time_info.bbt.bar =
    position_get_bars (PLAYHEAD, true);
  self->time_info.bbt.beat =
    position_get_beats (PLAYHEAD, true);
  self->time_info.bbt.tick =
    position_get_sixteenths (PLAYHEAD, true) *
      TICKS_PER_SIXTEENTH_NOTE +
    (int) floor (position_get_ticks (PLAYHEAD));
  Position bar_start;
  position_set_to_bar (
    &bar_start,
    position_get_bars (PLAYHEAD, true));
  self->time_info.bbt.barStartTick =
    (double)
    (PLAYHEAD->ticks - bar_start.ticks);
  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int beat_unit =
    tempo_track_get_beat_unit (P_TEMPO_TRACK);
  self->time_info.bbt.beatsPerBar =
    (float) beats_per_bar;
  self->time_info.bbt.beatType = (float) beat_unit;
  self->time_info.bbt.ticksPerBeat =
    TRANSPORT->ticks_per_beat;
  self->time_info.bbt.beatsPerMinute =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);

  const PluginDescriptor * descr =
    self->plugin->setting->descr;
  switch (descr->protocol)
    {
    case PROT_LV2:
    case PROT_VST:
    case PROT_VST3:
    case PROT_DSSI:
    case PROT_LADSPA:
    case PROT_AU:
    case PROT_SFZ:
    case PROT_SF2:
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
                &self->plugin->in_ports[i]->buf[
                  local_offset];
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
                &self->plugin->out_ports[i]->buf[
                  local_offset];
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
          if (port->id.type == TYPE_EVENT &&
              port->id.flags2 &
                PORT_FLAG2_SUPPORTS_MIDI)
            {
              break;
            }
          else
            port = NULL;
        }

      int num_events =
        port ? port->midi_events->num_events : 0;
#define MAX_EVENTS 4000
      NativeMidiEvent events[MAX_EVENTS];
      int num_events_written = 0;
      for (i = 0; i < num_events; i++)
        {
          MidiEvent * ev =
            &port->midi_events->events[i];
          if (ev->time < local_offset ||
              ev->time >= local_offset + nframes)
            {
              /* skip events scheduled
               * for another split within
               * the processing cycle */
              continue;
            }

#if 0
          g_message (
            "writing plugin input event %d "
            "at time %u - "
            "local frames %u nframes %u",
            num_events_written,
            ev->time - local_offset,
            local_offset, nframes);
          midi_event_print (ev);
#endif

          /* event time is relative to the current
           * zrythm full cycle (not split). it
           * needs to be made relative to the
           * current split */
          events[num_events_written].time =
            ev->time - local_offset;
          events[num_events_written].size = 3;
          events[num_events_written].data[0] =
            ev->raw_buffer[0];
          events[num_events_written].data[1] =
            ev->raw_buffer[1];
          events[num_events_written].data[2] =
            ev->raw_buffer[2];
          num_events_written++;

          if (num_events_written == MAX_EVENTS)
            {
              g_warning (
                "written %d events", MAX_EVENTS);
              break;
            }
        }
      if (num_events_written > 0)
        {
#if 0
          g_message (
            "Carla plugin %s has %d MIDI events",
            self->plugin->descr->name,
            num_events_written);
#endif
        }

      /*g_warn_if_reached ();*/
      self->native_plugin_descriptor->process (
        self->native_plugin_handle, inbuf, outbuf,
        nframes, events,
        (uint32_t) num_events_written);
      }
      break;
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
    case PLUGIN_CATEGORY_NONE:
    default:
      return g_strdup ("Plugin");
      break;
    }
  g_return_val_if_reached (NULL);
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
    plugin_descriptor_new ();

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
  descr->min_bridge_mode =
    plugin_descriptor_get_min_bridge_mode (descr);
  descr->has_custom_ui =
    info->hints & PLUGIN_HAS_CUSTOM_UI;

  return descr;
}

/**
 * Creates an instance of a CarlaNativePlugin inside
 * the given Plugin.
 *
 * The given Plugin must have its descriptor
 * filled in.
 *
 * @return Non-zero if fail.
 */
int
carla_native_plugin_new_from_setting (
  Plugin *  plugin,
  GError ** error)
{
  g_return_val_if_fail (
    plugin->setting->open_with_carla, -1);

  CarlaNativePlugin * self =
    object_new (CarlaNativePlugin);

  plugin->carla = self;
  self->plugin = plugin;

  return 0;
}

static void
create_ports (
  CarlaNativePlugin * self,
  bool                loading)
{
  g_debug ("%s: loading: %d", __func__, loading);

  char tmp[500];
  char name[4000];

  const PluginDescriptor * descr =
    self->plugin->setting->descr;
  if (!loading)
    {
      for (int i = 0; i < descr->num_audio_ins; i++)
        {
          strcpy (tmp, _("Audio in"));
          sprintf (name, "%s %d", tmp, i);
          Port * port =
            port_new_with_type (
              TYPE_AUDIO, FLOW_INPUT, name);
          plugin_add_in_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_audio_outs; i++)
        {
          strcpy (tmp, _("Audio out"));
          sprintf (name, "%s %d", tmp, i);
          Port * port =
            port_new_with_type (
              TYPE_AUDIO, FLOW_OUTPUT, name);
          plugin_add_out_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_midi_ins; i++)
        {
          strcpy (tmp, _("MIDI in"));
          sprintf (name, "%s %d", tmp, i);
          Port * port =
            port_new_with_type (
              TYPE_EVENT, FLOW_INPUT, name);
          port->id.flags2 |=
            PORT_FLAG2_SUPPORTS_MIDI;
          plugin_add_in_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_midi_outs; i++)
        {
          strcpy (tmp, _("MIDI out"));
          sprintf (name, "%s %d", tmp, i);
          Port * port =
            port_new_with_type (
              TYPE_EVENT, FLOW_OUTPUT, name);
          port->id.flags2 |=
            PORT_FLAG2_SUPPORTS_MIDI;
          plugin_add_out_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_cv_ins; i++)
        {
          strcpy (tmp, _("CV in"));
          sprintf (name, "%s %d", tmp, i);
          Port * port =
            port_new_with_type (
              TYPE_CV, FLOW_INPUT, name);
          plugin_add_in_port (
            self->plugin, port);
        }
      for (int i = 0; i < descr->num_cv_outs; i++)
        {
          strcpy (tmp, _("CV out"));
          sprintf (name, "%s %d", tmp, i);
          Port * port =
            port_new_with_type (
              TYPE_CV, FLOW_OUTPUT, name);
          plugin_add_out_port (
            self->plugin, port);
        }
    }

  /* create controls */
  const CarlaPortCountInfo * param_counts =
    carla_get_parameter_count_info (
      self->host_handle, 0);
#if 0
  /* FIXME eventually remove this line. this is added
   * because carla discovery reports 0 params for
   * AU plugins, so we update the descriptor here */
  descr->num_ctrl_ins = (int) param_counts->ins;
  g_message (
    "params: %d ins and %d outs",
    descr->num_ctrl_ins, (int) param_counts->outs);
#endif
  for (uint32_t i = 0; i < param_counts->ins; i++)
    {
      Port * port = NULL;
      if (loading)
        {
          port =
            carla_native_plugin_get_port_from_param_id (
              self, i);
          g_return_if_fail (
            IS_PORT_AND_NONNULL (port));
        }
      /* else if not loading (create new ports) */
      else
        {
          const CarlaParameterInfo * param_info =
            carla_get_parameter_info (
              self->host_handle, 0, i);
          port =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT,
              param_info->name);
          g_return_if_fail (
            IS_PORT_AND_NONNULL (port));
          if (param_info->symbol &&
              strlen (param_info->symbol) > 0)
            {
              port->id.sym =
                g_strdup (param_info->symbol);
            }
          port->id.flags |=
            PORT_FLAG_PLUGIN_CONTROL;
          if (param_info->comment &&
              strlen (param_info->comment) > 0)
            {
              port->id.comment =
                g_strdup (param_info->comment);
            }
          if (param_info->groupName &&
              strlen (param_info->groupName) > 0)
            {
              port->id.port_group =
                string_get_substr_before_suffix (
                  param_info->groupName, ":");
              g_return_if_fail (
                port->id.port_group);
            }
          port->carla_param_id = (int) i;

          const NativeParameter * native_param =
            self->native_plugin_descriptor->
              get_parameter_info (
                self->native_plugin_handle, i);
          g_return_if_fail (native_param);
          if (native_param->hints &
                NATIVE_PARAMETER_IS_LOGARITHMIC)
            {
              port->id.flags |=
                PORT_FLAG_LOGARITHMIC;
            }
          if (native_param->hints &
                NATIVE_PARAMETER_IS_AUTOMABLE)
            {
              port->id.flags |=
                PORT_FLAG_AUTOMATABLE;
            }
          if (native_param->hints &
                NATIVE_PARAMETER_IS_BOOLEAN)
            {
              port->id.flags |= PORT_FLAG_TOGGLE;
            }
          else if (native_param->hints &
                     NATIVE_PARAMETER_IS_INTEGER)
            {
              port->id.flags |= PORT_FLAG_INTEGER;
            }

          /* get scale points */
          if (param_info->scalePointCount > 0)
            {
              port->scale_points =
                object_new_n (
                  param_info->scalePointCount,
                  PortScalePoint);
              port->num_scale_points =
                (int) param_info->scalePointCount;
            }
          for (uint32_t j = 0;
               j < param_info->scalePointCount; j++)
            {
              const CarlaScalePointInfo *
                scale_point_info =
                  carla_get_parameter_scalepoint_info (
                    self->host_handle, 0, i, j);

              port->scale_points[j] =
                port_scale_point_new (
                  scale_point_info->value,
                  scale_point_info->label);
            }
          qsort (
            port->scale_points,
            (size_t) port->num_scale_points,
            sizeof (PortScalePoint *),
            port_scale_point_cmp);

          plugin_add_in_port (
            self->plugin, port);

          for (uint32_t j = 0;
               j < param_info->scalePointCount; j++)
            {
              const CarlaScalePointInfo * sp_info =
                carla_get_parameter_scalepoint_info (
                  self->host_handle, 0, i, j);
              g_debug (
                "scale point: %s", sp_info->label);
            }

          const ParameterRanges * ranges =
            carla_get_parameter_ranges (
              self->host_handle, 0, i);
          port->deff = ranges->def;
          port->minf = ranges->min;
          port->maxf = ranges->max;
#if 0
          g_debug (
            "ranges: min %f max %f default %f",
            (double) port->minf,
            (double) port->maxf,
            (double) port->deff);
#endif
        }
      float cur_val =
        carla_native_plugin_get_param_value (
          self, i);
      g_debug (
        "%d: %s=%f%s", i, port->id.label,
        (double) cur_val,
        loading ? " (loading)" : "");
      port_set_control_value (
        port, cur_val, F_NOT_NORMALIZED,
        F_NO_PUBLISH_EVENTS);
    }

  self->ports_created = true;
}

/**
 * Instantiates the plugin.
 *
 * @param loading Whether loading an existing plugin
 *   or not.
 * @param use_state_file Whether to use the plugin's
 *   state file to instantiate the plugin.
 *
 * @return 0 if no errors, non-zero if errors.
 */
int
carla_native_plugin_instantiate (
  CarlaNativePlugin * self,
  bool                loading,
  bool                use_state_file,
  GError **           error)
{
  g_debug (
    "loading: %i, use state file: %d, "
    "ports_created: %d",
    loading, use_state_file, self->ports_created);

  self->native_host_descriptor.handle = self;
  self->native_host_descriptor.uiName =
    g_strdup ("Zrythm");

  self->native_host_descriptor.uiParentId = 0;

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
  self->native_host_descriptor.
    ui_parameter_changed =
      host_ui_parameter_changed;
  self->native_host_descriptor.
    ui_custom_data_changed =
      host_ui_custom_data_changed;
  self->native_host_descriptor.ui_closed =
    host_ui_closed;
  self->native_host_descriptor.ui_open_file =
    NULL;
  self->native_host_descriptor.ui_save_file =
    NULL;
  self->native_host_descriptor.dispatcher = host_dispatcher;

  self->time_info.bbt.valid = 1;

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
  char * zrythm_libdir =
    zrythm_get_dir (
      ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR);
  char * carla_binaries_dir =
    g_build_filename (
      zrythm_libdir, "carla", NULL);
  g_message (
    "setting carla engine option "
    "[ENGINE_OPTION_PATH_BINARIES] to '%s'",
    carla_binaries_dir);
  carla_set_engine_option (
    self->host_handle,
    ENGINE_OPTION_PATH_BINARIES, 0,
    carla_binaries_dir);
  g_free (zrythm_libdir);
  g_free (carla_binaries_dir);

  /* set lv2 path */
  carla_set_engine_option (
    self->host_handle,
    ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2,
    PLUGIN_MANAGER->lv2_path);

  /* set UI scale factor */
  carla_set_engine_option (
    self->host_handle,
    ENGINE_OPTION_FRONTEND_UI_SCALE,
    (int)
    ((float) self->plugin->ui_scale_factor *
       1000.f),
    NULL);

  /* set whether UI should stay on top */
  carla_set_engine_option (
    self->host_handle,
    ENGINE_OPTION_FRONTEND_UI_SCALE,
    (int)
    ((float) self->plugin->ui_scale_factor *
       1000.f),
    NULL);

  if (!ZRYTHM_TESTING &&
      g_settings_get_boolean (
        S_P_PLUGINS_UIS, "stay-on-top"))
    {
      carla_set_engine_option (
        self->host_handle,
        ENGINE_OPTION_UIS_ALWAYS_ON_TOP, true,
        NULL);
    }

  const PluginSetting * setting =
    self->plugin->setting;
  g_return_val_if_fail (
    setting->open_with_carla, -1);
  const PluginDescriptor * descr = setting->descr;
  g_message (
    "%s: using bridge mode %s", __func__,
    carla_bridge_mode_strings[
      setting->bridge_mode].str);

  /* set bridging on if needed */
  switch (setting->bridge_mode)
    {
    case CARLA_BRIDGE_FULL:
      g_message (
        "plugin must be bridged whole, "
        "using plugin bridge");
      carla_set_engine_option (
        self->host_handle,
        ENGINE_OPTION_PREFER_PLUGIN_BRIDGES,
        true, NULL);
      break;
    case CARLA_BRIDGE_UI:
      g_message ("using UI bridge only");
      carla_set_engine_option (
        self->host_handle,
        ENGINE_OPTION_PREFER_UI_BRIDGES,
        true, NULL);
      break;
    default:
      break;
    }

  /* raise bridge timeout to 8 sec */
  if (setting->bridge_mode == CARLA_BRIDGE_FULL ||
      setting->bridge_mode == CARLA_BRIDGE_UI)
    {
      carla_set_engine_option (
        self->host_handle,
        ENGINE_OPTION_UI_BRIDGES_TIMEOUT,
        8000, NULL);
    }

  const PluginType type =
    get_plugin_type_from_protocol (descr->protocol);
  int ret = 0;
  switch (descr->protocol)
    {
    case PROT_LV2:
    case PROT_AU:
      g_message ("uri %s", descr->uri);
      ret =
        carla_add_plugin (
          self->host_handle,
          descr->arch == ARCH_64 ?
            BINARY_NATIVE : BINARY_WIN32,
          type, NULL, descr->name,
          descr->uri, 0, NULL, 0);
      break;
    case PROT_VST:
    case PROT_VST3:
      ret =
        carla_add_plugin (
          self->host_handle,
          descr->arch == ARCH_64 ?
            BINARY_NATIVE : BINARY_WIN32,
          type, descr->path, descr->name,
          descr->name, descr->unique_id, NULL, 0);
      break;
    case PROT_DSSI:
    case PROT_LADSPA:
      ret =
        carla_add_plugin (
          self->host_handle, BINARY_NATIVE,
          type, descr->path, descr->name,
          descr->uri, 0, NULL, 0);
      break;
    case PROT_SFZ:
    case PROT_SF2:
      ret =
        carla_add_plugin (
          self->host_handle,
          BINARY_NATIVE,
          type, descr->path, descr->name,
          descr->name, 0, NULL, 0);
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  if (ret != 1)
    {
      g_set_error (
        error,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
        _("Error adding carla plugin: %s"),
        carla_get_last_error (self->host_handle));
      return -1;
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

#undef ENABLE_OPTION

  /* add engine callback */
  carla_set_engine_callback (
    self->host_handle, engine_callback, self);

  if (use_state_file)
    {
      /* load the state */
      GError * err = NULL;
      bool state_loaded =
        carla_native_plugin_load_state (
          self->plugin->carla, NULL, &err);
      if (!state_loaded)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s",
            _("Failed to load Carla state"));
          self->plugin->instantiation_failed =
            true;
          return -1;
        }
    }

  if (!self->native_plugin_handle
      || !self->native_plugin_descriptor->activate
      || !self->native_plugin_descriptor->ui_show)
    {
      g_set_error (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
        "%s",
        _("Failed to instantiate Carla plugin: "
        "handle/descriptor not initialized "
        "properly"));
      self->plugin->instantiation_failed =
        true;
      return -1;
    }

  /* create ports */
  if (!loading && !use_state_file &&
      !self->ports_created)
    {
      create_ports (self, false);
    }

  g_message ("activating carla plugin...");
  self->native_plugin_descriptor->activate (
    self->native_plugin_handle);
  g_message ("carla plugin activated");

  /* load data into existing ports */
  if (loading || use_state_file)
    {
      create_ports (self, true);
    }

  return 0;
}

static void
carla_plugin_tick_cb_destroy (
  void * data)
{
  g_return_if_fail (data);
  CarlaNativePlugin * self =
    (CarlaNativePlugin *) data;

  self->tick_cb = 0;
}

/**
 * Shows or hides the UI.
 */
void
carla_native_plugin_open_ui (
  CarlaNativePlugin * self,
  bool                show)
{
  Plugin * pl = self->plugin;
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));
  g_return_if_fail (
    plugin_is_in_active_project (pl));
  g_return_if_fail (pl->setting->descr);

  g_message (
    "%s: show/hide '%s (%p)' UI: %d",
    __func__, pl->setting->descr->name, pl, show);

  if ((self->tick_cb == 0 && !show)
      || (self->tick_cb != 0 && show))
    {
      g_message (
        "plugin already has visibility status %d, "
        "doing nothing",
        show);
      return;
    }

  switch (pl->setting->descr->protocol)
    {
    case PROT_VST:
    case PROT_VST3:
    case PROT_DSSI:
    case PROT_LV2:
    case PROT_AU:
      if (show)
        {
          char * title =
            plugin_generate_window_title (pl);
          g_debug ("plugin window title '%s'", title);
          carla_set_custom_ui_title (
            self->host_handle, 0, title);
          g_free (title);

          /* set whether to keep window on top */
          if (ZRYTHM_HAVE_UI &&
              g_settings_get_boolean (
                S_P_PLUGINS_UIS, "stay-on-top"))
            {
#ifdef HAVE_X11
              Window xid =
                z_gtk_window_get_x11_xid (
                  GTK_WINDOW (MAIN_WINDOW));
              char xid_str[400];
              sprintf (xid_str, "%lx", xid);
              carla_set_engine_option (
                self->host_handle,
                ENGINE_OPTION_FRONTEND_WIN_ID, 0,
                xid_str);
#endif
            }
        }

      carla_show_custom_ui (
        self->host_handle, 0, show);
      pl->visible = show;

      if (self->tick_cb)
        {
          g_debug (
            "removing tick callback for %s",
            pl->setting->descr->name);
          gtk_widget_remove_tick_callback (
            GTK_WIDGET (MAIN_WINDOW),
            self->tick_cb);
          self->tick_cb = 0;
        }

      if (show)
        {
          g_return_if_fail (MAIN_WINDOW);
          g_debug (
            "setting tick callback for %s",
            pl->setting->descr->name);
          self->tick_cb =
            gtk_widget_add_tick_callback (
              GTK_WIDGET (MAIN_WINDOW),
              (GtkTickCallback)
              carla_plugin_tick_cb,
              self,
              carla_plugin_tick_cb_destroy);
        }

      if (!ZRYTHM_TESTING)
        {
          EVENTS_PUSH (
            ET_PLUGIN_WINDOW_VISIBILITY_CHANGED,
            pl);
        }
      break;
    default:
      break;
    }
}

int
carla_native_plugin_activate (
  CarlaNativePlugin * self,
  bool                activate)
{
  g_message ("setting plugin %s active %d",
    self->plugin->setting->descr->name, activate);
  carla_set_active (
    self->host_handle, 0, activate);

  return 0;
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

/**
 * Called from port_set_control_value() to send
 * the value to carla.
 *
 * @param val Real value (ie, not normalized).
 */
void
carla_native_plugin_set_param_value (
  CarlaNativePlugin * self,
  const uint32_t      id,
  float               val)
{
  if (self->plugin->instantiation_failed)
    {
      return;
    }

  float cur_val =
    carla_get_current_parameter_value (
      self->host_handle, 0, id);
  if (!math_floats_equal (cur_val, val))
    {
      g_debug (
        "setting param %d value to %f",
        id, (double) val);
    }
  carla_set_parameter_value (
    self->host_handle, 0, id, val);
}

/**
 * Returns the MIDI out port.
 */
Port *
carla_native_plugin_get_midi_out_port (
  CarlaNativePlugin * self)
{
  Plugin * pl = self->plugin;
  Port * port;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->out_ports[i];
      if (port->id.type == TYPE_EVENT &&
          port->id.flags2 &
            PORT_FLAG2_SUPPORTS_MIDI)
        return port;
    }

  g_return_val_if_reached (NULL);
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
  int j = 0;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];
      if (port->id.type != TYPE_CONTROL)
        continue;

      j = port->carla_param_id;
      if ((int) id == port->carla_param_id)
        return port;
    }

  if ((int) id > j)
    {
      g_message (
        "%s: index %u not found in input ports. "
        "this is likely an output param. ignoring",
        __func__, id);
      return NULL;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Saves the state inside the standard state
 * directory.
 *
 * @param is_backup Whether this is a backup
 *   project. Used for calculating the absolute
 *   path to the state dir.
 * @param abs_state_dir If passed, the state will
 *   be saved inside this directory instead of the
 *   plugin's state directory. Used when saving
 *   presets.
 */
int
carla_native_plugin_save_state (
  CarlaNativePlugin * self,
  bool                is_backup,
  const char *        abs_state_dir)
{
  if (!self->plugin->instantiated)
    {
      g_debug (
        "plugin %s not instantiated, skipping %s",
        self->plugin->setting->descr->name,
        __func__);
      return 0;
    }

  char * dir_to_use = NULL;
  if (abs_state_dir)
    {
      dir_to_use = g_strdup (abs_state_dir);
    }
  else
    {
      dir_to_use =
        plugin_get_abs_state_dir (
          self->plugin, is_backup);
    }
  io_mkdir (dir_to_use);
  char * state_file_abs_path =
    g_build_filename (
      dir_to_use, CARLA_STATE_FILENAME, NULL);
  carla_save_plugin_state (
    self->host_handle, 0, state_file_abs_path);
  g_free (state_file_abs_path);

  g_warn_if_fail (self->plugin->state_dir);

  return 0;
}

char *
carla_native_plugin_get_abs_state_file_path (
  const CarlaNativePlugin * self,
  const bool                is_backup)
{
  char * abs_state_dir =
    plugin_get_abs_state_dir (
      self->plugin, is_backup);
  char * state_file_abs_path =
    g_build_filename (
      abs_state_dir, CARLA_STATE_FILENAME, NULL);

  g_free (abs_state_dir);

  return state_file_abs_path;
}

/**
 * Loads the state from the given file or from
 * its state file.
 *
 * @return True on success.
 */
bool
carla_native_plugin_load_state (
  CarlaNativePlugin * self,
  const char *        abs_path,
  GError **           error)
{
  Plugin * pl = self->plugin;
  g_return_val_if_fail (
    IS_PLUGIN_AND_NONNULL (pl), false);

  g_debug (
    "%s: loading state from %s...",
    __func__, abs_path);
  char * state_file;
  if (abs_path)
    {
      state_file = g_strdup (abs_path);
    }
  else
    {
      char * state_dir_abs_path =
        plugin_get_abs_state_dir (
          pl,
          PROJECT->loading_from_backup);
      state_file =
        g_build_filename (
          state_dir_abs_path, CARLA_STATE_FILENAME,
          NULL);
      g_free (state_dir_abs_path);
    }

  if (!file_exists (state_file))
    {
      g_set_error (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_FAILED,
        _("State file %s doesn't exist"),
        state_file);
      return false;
    }

  self->loading_state = true;
  carla_load_plugin_state (
    self->host_handle, 0, state_file);
  self->loading_state = false;
  if (pl->visible
      && plugin_is_in_active_project (pl))
    {
      EVENTS_PUSH (
        ET_PLUGIN_VISIBILITY_CHANGED, pl);
    }
  g_message (
    "%s: successfully loaded carla plugin state "
    "from %s",
    __func__, state_file);

  g_free (state_file);

  return true;
}

void
carla_native_plugin_close (
  CarlaNativePlugin * self)
{
  if (self->native_plugin_descriptor)
    {
      self->native_plugin_descriptor->deactivate (
        self->native_plugin_handle);
      self->native_plugin_descriptor->cleanup (
        self->native_plugin_handle);
      self->native_plugin_descriptor = NULL;
    }
  if (self->host_handle)
    {
      carla_host_handle_free (self->host_handle);
      self->host_handle = NULL;
    }
}

/**
 * Deactivates, cleanups and frees the instance.
 */
void
carla_native_plugin_free (
  CarlaNativePlugin * self)
{
  carla_native_plugin_close (self);

  object_zero_and_free (self);
}
#endif
