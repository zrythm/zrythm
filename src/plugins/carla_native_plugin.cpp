// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>

#ifdef HAVE_CARLA

#  include "dsp/engine.h"
#  include "dsp/midi_event.h"
#  include "dsp/tempo_track.h"
#  include "dsp/transport.h"
#  include "gui/backend/event.h"
#  include "gui/backend/event_manager.h"
#  include "gui/widgets/main_window.h"
#  include "plugins/carla_discovery.h"
#  include "plugins/carla_native_plugin.h"
#  include "plugins/plugin.h"
#  include "plugins/plugin_manager.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/debug.h"
#  include "utils/dsp.h"
#  include "utils/error.h"
#  include "utils/file.h"
#  include "utils/flags.h"
#  include "utils/gtk.h"
#  include "utils/io.h"
#  include "utils/math.h"
#  include "utils/objects.h"
#  include "utils/string.h"
#  include "zrythm.h"
#  include "zrythm_app.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include <CarlaHost.h>
#  include <CarlaNative.h>
#  include <fmt/format.h>
#  include <fmt/printf.h>

typedef enum
{
  Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
  Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_FAILED,
} ZPluginsCarlaNativePluginError;

#  define Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR \
    z_plugins_carla_native_plugin_error_quark ()
GQuark
z_plugins_carla_native_plugin_error_quark (void);
G_DEFINE_QUARK (
  z - plugins - carla - native - plugin - error - quark,
  z_plugins_carla_native_plugin_error)

void
carla_native_plugin_init_loaded (CarlaNativePlugin * self)
{
}

static GdkGLContext *
clear_gl_context (void)
{
  if (string_is_equal (z_gtk_get_gsk_renderer_type (), "GL"))
    {
      GdkGLContext * context = gdk_gl_context_get_current ();
      if (context)
        {
          /*g_warning ("have GL context - clearing");*/
          g_object_ref (context);
          gdk_gl_context_clear_current ();
        }
      return context;
    }

  return NULL;
}

static void
return_gl_context (GdkGLContext * context)
{
  if (context)
    {
      /*g_debug ("returning context");*/
      gdk_gl_context_make_current (context);
      g_object_unref (context);
    }
}

/**
 * Tick callback for the plugin UI.
 */
static gboolean
carla_plugin_tick_cb (gpointer user_data)
{
  CarlaNativePlugin * self = (CarlaNativePlugin *) user_data;
  if (self->plugin->visible && MAIN_WINDOW)
    {
      GdkGLContext * context = clear_gl_context ();
      /*g_debug ("calling ui_idle()...");*/
      self->native_plugin_descriptor->ui_idle (self->native_plugin_handle);
      /*g_debug ("done calling ui_idle()");*/
      return_gl_context (context);

      return G_SOURCE_CONTINUE;
    }
  else
    return G_SOURCE_REMOVE;
}

static uint32_t
host_get_buffer_size (NativeHostHandle handle)
{
  uint32_t buffer_size = 512;
  if (PROJECT && AUDIO_ENGINE && AUDIO_ENGINE->block_length > 0)
    buffer_size = (uint32_t) AUDIO_ENGINE->block_length;

  return buffer_size;
}

static double
host_get_sample_rate (NativeHostHandle handle)
{
  double sample_rate = 44000.0;
  if (PROJECT && AUDIO_ENGINE && AUDIO_ENGINE->sample_rate > 0)
    sample_rate = (double) AUDIO_ENGINE->sample_rate;

  return sample_rate;
}

static bool
host_is_offline (NativeHostHandle handle)
{
  if (!PROJECT || !AUDIO_ENGINE)
    {
      return true;
    }

  return !AUDIO_ENGINE->run;
}

static const NativeTimeInfo *
host_get_time_info (NativeHostHandle handle)
{
  CarlaNativePlugin * plugin = (CarlaNativePlugin *) handle;
  return &plugin->time_info;
}

static bool
host_write_midi_event (NativeHostHandle handle, const NativeMidiEvent * event)
{
  /*g_message ("write midi event");*/

  CarlaNativePlugin * self = (CarlaNativePlugin *) handle;

  Port * midi_out_port = carla_native_plugin_get_midi_out_port (self);
  g_return_val_if_fail (IS_PORT_AND_NONNULL (midi_out_port), 0);

  midi_byte_t buf[event->size];
  for (int i = 0; i < event->size; i++)
    {
      buf[i] = event->data[i];
    }
  midi_events_add_event_from_buf (
    midi_out_port->midi_events_, event->time, buf, event->size, false);

  return 0;
}

static void
host_ui_parameter_changed (NativeHostHandle handle, uint32_t index, float value)
{
  /*g_debug ("handle ui param changed");*/
  CarlaNativePlugin * self = (CarlaNativePlugin *) handle;
  Port * port = carla_native_plugin_get_port_from_param_id (self, index);
  if (!port)
    {
      g_message ("%s: no port found for param %u", __func__, index);
      return;
    }

  if (carla_get_current_plugin_count (self->host_handle) == 2)
    {
      carla_set_parameter_value (self->host_handle, 1, index, value);
    }

  port->set_control_value (value, false, false);
}

static void
host_ui_custom_data_changed (
  NativeHostHandle handle,
  const char *     key,
  const char *     value)
{
}

static void
host_ui_closed (NativeHostHandle handle)
{
  CarlaNativePlugin * self = (CarlaNativePlugin *) handle;

  g_message ("ui closed");
  if (self->tick_cb)
    {
      g_source_remove (self->tick_cb);
      self->tick_cb = 0;
    }
}

static intptr_t
host_dispatcher (
  NativeHostHandle           handle,
  NativeHostDispatcherOpcode opcode,
  int32_t                    index,
  intptr_t                   value,
  void *                     ptr,
  float                      opt)
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
#  if 0
      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }
#  endif
      break;
#  if 0
    case NATIVE_HOST_OPCODE_Port::InternalType::INTERNAL_PLUGIN:
      /* falktx: you will need to call the new
       * juce functions, then return 1 on the
       * Port::InternalType::INTERNAL_PLUGIN host opcode.
       * when that opcode returns 1, carla-plugin
       * will let the host do the juce idling
       * which is for the best here, since you want
       * to show UIs without carla itself */
      return 1;
      break;
#  endif
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
carla_engine_callback (
  void *               ptr,
  EngineCallbackOpcode action,
  uint                 plugin_id,
  int                  val1,
  int                  val2,
  int                  val3,
  float                valf,
  const char *         val_str)
{
  CarlaNativePlugin * self = (CarlaNativePlugin *) ptr;

  switch (action)
    {
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_ADDED:
      g_message ("Plugin added: %u - %s", plugin_id, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_REMOVED:
      g_message ("Plugin removed: %u", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_RENAMED:
      break;
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
      g_warning ("Plugin unavailable: %u - %s", plugin_id, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
      /* if plugin was deactivated and we didn't explicitly tell it to
       * deactivate */
      if (
        val1 == CarlaBackend::PARAMETER_ACTIVE && val2 == 0 && val3 == 0
        && self->plugin->activated && !self->plugin->deactivating
        && !self->loading_state)
        {
          /* send crash signal */
          EVENTS_PUSH (EventType::ET_PLUGIN_CRASHED, self->plugin);
        }
      break;
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED:
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
      break;
    case CarlaBackend::ENGINE_CALLBACK_OPTION_CHANGED:
      g_message (
        "Option changed: plugin %u - opt %d: %d", plugin_id, val1, val2);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PROGRAM_CHANGED:
      g_message ("Program changed: plugin %u - %d", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED:
      g_message ("MIDI program changed: plugin %u - %d", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED:
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
      EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, self->plugin);
      break;
    case CarlaBackend::ENGINE_CALLBACK_NOTE_ON:
    case CarlaBackend::ENGINE_CALLBACK_NOTE_OFF:
      break;
    case CarlaBackend::ENGINE_CALLBACK_UPDATE:
      g_debug ("plugin %u needs update", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_INFO:
      g_debug ("plugin %u reload info", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_PARAMETERS:
      g_debug ("plugin %u reload parameters", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_PROGRAMS:
      g_debug ("plugin %u reload programs", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_ALL:
      g_debug ("plugin %u reload all", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
      g_debug (
        "Patchbay client added: %u plugin %d name %s", plugin_id, val2, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED:
      g_debug ("Patchbay client removed: %u", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED:
      g_debug ("Patchbay client renamed: %u - %s", plugin_id, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED:
      g_debug (
        "Patchbay client data changed: %u - %d %d", plugin_id, val1, val2);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
      {
        g_debug (
          "PORT ADDED: client %u port %d "
          "group %d name %s",
          plugin_id, val1, val3, val_str);
        bool is_cv_variant =
          self->max_variant_cv_ins > 0 || self->max_variant_cv_outs > 0;
        unsigned int port_id = (unsigned int) val1;
        switch (plugin_id)
          {
          case 1:
            if (self->audio_input_port_id == 0)
              self->audio_input_port_id = port_id;
            break;
          case 2:
            if (self->audio_output_port_id == 0)
              self->audio_output_port_id = port_id;
            break;
          case 3:
            if (is_cv_variant && self->cv_input_port_id == 0)
              {
                self->cv_input_port_id = port_id;
              }
            else if (!is_cv_variant && self->midi_input_port_id == 0)
              {
                self->midi_input_port_id = port_id;
              }
            break;
          case 4:
            if (is_cv_variant && self->cv_output_port_id == 0)
              {
                self->cv_output_port_id = port_id;
              }
            else if (!is_cv_variant && self->midi_output_port_id == 0)
              {
                self->midi_output_port_id = port_id;
              }
            break;
          case 5:
            if (is_cv_variant && self->midi_input_port_id == 0)
              {
                self->midi_input_port_id = port_id;
              }
            break;
          case 6:
            if (is_cv_variant && self->midi_output_port_id == 0)
              {
                self->midi_output_port_id = port_id;
              }
            break;
          default:
            break;
          }

        /* if non-cv variant, there will be no CV
         * clients */
        if (
          (is_cv_variant && plugin_id >= 7)
          || (!is_cv_variant && plugin_id >= 5))
          {
            unsigned int            port_hints = (unsigned int) val2;
            CarlaPatchbayPortInfo * nfo = object_new (CarlaPatchbayPortInfo);
            nfo->plugin_id = plugin_id;
            nfo->port_hints = port_hints;
            nfo->port_id = port_id;
            nfo->port_name = g_strdup (val_str);
            g_ptr_array_add (self->patchbay_port_info, nfo);
          }
      }
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
      g_debug ("Patchbay port removed: %u - %d", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED:
      g_debug ("Patchbay port changed: %u - %d", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
      g_debug ("Connection added %s", val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
      g_debug ("Connection removed");
      break;
    case CarlaBackend::ENGINE_CALLBACK_ENGINE_STARTED:
      g_message ("Engine started");
      break;
    case CarlaBackend::ENGINE_CALLBACK_ENGINE_STOPPED:
      g_message ("Engine stopped");
      break;
    case CarlaBackend::ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
      g_debug ("Process mode changed: %d", val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
      g_debug ("Transport mode changed: %d - %s", val1, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
      g_message ("Buffer size changed: %d", val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
      g_message ("Sample rate changed: %f", (double) valf);
      break;
    case CarlaBackend::ENGINE_CALLBACK_CANCELABLE_ACTION:
      g_debug (
        "Cancelable action: plugin %u - %d - %s", plugin_id, val1, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PROJECT_LOAD_FINISHED:
      g_message ("Project load finished");
      break;
    case CarlaBackend::ENGINE_CALLBACK_NSM:
      break;
    /*!
     * Idle frontend.
     * This is used by the engine during long operations that might block the
     * frontend, giving it the possibility to idle while the operation is still
     * in place.
     */
    case CarlaBackend::ENGINE_CALLBACK_IDLE:
      /* TODO */
      break;
    case CarlaBackend::ENGINE_CALLBACK_INFO:
      g_message ("Engine info: %s", val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_ERROR:
      g_warning ("Engine error: %s", val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_QUIT:
      g_warning (
        "Engine quit: engine crashed or "
        "malfunctioned and will no longer work");
      break;
    /*!
     * A plugin requested for its inline display to be redrawn.
     * @a pluginId Plugin Id to redraw
     */
    case CarlaBackend::ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW:
      /* TODO */
      break;
    /*!
     * A patchbay port group has been added.
     * @a pluginId Client Id
     * @a value1   Group Id (unique value within this client)
     * @a value2   Group hints
     * @a valueStr Group name
     * @see PatchbayPortGroupHints
     */
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED:
      /* TODO */
      break;
    /*!
     * A patchbay port group has been removed.
     * @a pluginId Client Id
     * @a value1   Group Id
     */
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED:
      /* TODO */
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED:
      g_debug (
        "Patchbay port group changed: client %u - "
        "group %d - hints %d - name %s",
        plugin_id, val1, val2, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED:
      g_debug (
        "Parameter mapped range changed: %u:%d "
        "- %s",
        plugin_id, val1, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED:
      break;
    case CarlaBackend::ENGINE_CALLBACK_EMBED_UI_RESIZED:
      g_debug ("Embed UI resized: %u - %dx%d", plugin_id, val1, val2);
      break;
    default:
      break;
    }
}

void
carla_native_plugin_populate_banks (CarlaNativePlugin * self)
{
  /* add default bank and preset */
  PluginBank * pl_def_bank = plugin_add_bank_if_not_exists (
    self->plugin, PLUGIN_DEFAULT_BANK_URI, _ ("Default bank"));
  PluginPreset * pl_def_preset = plugin_preset_new ();
  pl_def_preset->uri = g_strdup (PLUGIN_INIT_PRESET_URI);
  pl_def_preset->name = g_strdup (_ ("Init"));
  plugin_add_preset_to_bank (self->plugin, pl_def_bank, pl_def_preset);

  GString * presets_gstr = g_string_new (NULL);

  uint32_t count = carla_get_program_count (self->host_handle, 0);
  for (uint32_t i = 0; i < count; i++)
    {
      PluginPreset * pl_preset = plugin_preset_new ();
      pl_preset->carla_program = (int) i;
      const char * program_name =
        carla_get_program_name (self->host_handle, 0, i);
      if (strlen (program_name) == 0)
        {
          pl_preset->name = g_strdup_printf (_ ("Preset %u"), i);
        }
      else
        {
          pl_preset->name = g_strdup (program_name);
        }
      plugin_add_preset_to_bank (self->plugin, pl_def_bank, pl_preset);

      g_string_append_printf (
        presets_gstr, "found preset %s (%d)\n", pl_preset->name,
        pl_preset->carla_program);
    }

  g_message ("found %d presets", count);

  char * str = g_string_free (presets_gstr, false);
  /*g_message ("%s", str);*/
  g_free (str);
}

bool
carla_native_plugin_has_custom_ui (const PluginDescriptor * descr)
{
#  if 0
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
    z_carla_discovery_get_plugin_type_from_protocol (descr->protocol);
  carla_add_plugin (
    native_pl->host_handle,
    descr->arch == ZPluginArchitecture::Z_PLUGIN_ARCHITECTURE_64 ?
      CarlaBackend::BINARY_NATIVE : CarlaBackend::BINARY_WIN32,
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
#  endif
  g_return_val_if_reached (false);
}

/**
 * Processes the plugin for this cycle.
 */
void
carla_native_plugin_process (
  CarlaNativePlugin *                 self,
  const EngineProcessTimeInfo * const time_nfo)
{
  self->time_info.playing = TRANSPORT_IS_ROLLING;
  self->time_info.frame = (uint64_t) time_nfo->g_start_frame_w_offset;
  self->time_info.bbt.bar = AUDIO_ENGINE->pos_nfo_current.bar;
  self->time_info.bbt.beat = AUDIO_ENGINE->pos_nfo_current.beat; // within bar
  self->time_info.bbt.tick =                                     // within beat
    AUDIO_ENGINE->pos_nfo_current.tick_within_beat;
  self->time_info.bbt.barStartTick =
    AUDIO_ENGINE->pos_nfo_current.tick_within_bar;
  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int beat_unit = tempo_track_get_beat_unit (P_TEMPO_TRACK);
  self->time_info.bbt.beatsPerBar = (float) beats_per_bar;
  self->time_info.bbt.beatType = (float) beat_unit;
  self->time_info.bbt.ticksPerBeat = TRANSPORT->ticks_per_beat;
  self->time_info.bbt.beatsPerMinute =
    tempo_track_get_current_bpm (P_TEMPO_TRACK);

  /* set actual audio in bufs */
  {
    size_t audio_ports = 0;
    for (
      size_t i = 0;
      i < self->plugin->audio_in_ports->len && i < self->max_variant_audio_ins;
      i++)
      {
        Port * port =
          (Port *) g_ptr_array_index (self->plugin->audio_in_ports, i);
        self->inbufs[audio_ports++] = &port->buf_[time_nfo->local_offset];
      }
  }

  /* set actual cv in bufs */
  {
    size_t cv_ports = 0;
    for (
      size_t i = 0;
      i < self->plugin->cv_in_ports->len && i < self->max_variant_cv_ins; i++)
      {
        Port * port = (Port *) g_ptr_array_index (self->plugin->cv_in_ports, i);
        self->inbufs[self->max_variant_audio_ins + cv_ports++] =
          &port->buf_[time_nfo->local_offset];
      }
  }

  /* set actual audio out bufs (carla will write to these) */
  {
    size_t audio_ports = 0;
    for (int i = 0; i < self->plugin->num_out_ports; i++)
      {
        Port * port = self->plugin->out_ports[i];
        if (port->id_.type_ == PortType::Audio)
          {
            self->outbufs[audio_ports++] = &port->buf_[time_nfo->local_offset];
          }
        if (audio_ports == self->max_variant_audio_outs)
          break;
      }
  }

  /* set actual cv out bufs (carla will write
   * to these) */
  {
    size_t cv_ports = 0;
    for (int i = 0; i < self->plugin->num_out_ports; i++)
      {
        Port * port = self->plugin->out_ports[i];
        if (port->id_.type_ == PortType::CV)
          {
            self->outbufs[self->max_variant_audio_outs + cv_ports++] =
              &port->buf_[time_nfo->local_offset];
          }
        if (cv_ports == self->max_variant_cv_outs)
          break;
      }
  }

  /* get main midi port */
  Port * port = self->plugin->midi_in_port;

  int num_events = port ? port->midi_events_->num_events : 0;
#  define MAX_EVENTS 4000
  NativeMidiEvent events[MAX_EVENTS];
  int             num_events_written = 0;
  for (int i = 0; i < num_events; i++)
    {
      MidiEvent * ev = &port->midi_events_->events[i];
      if (
        ev->time < time_nfo->local_offset
        || ev->time >= time_nfo->local_offset + time_nfo->nframes)
        {
          /* skip events scheduled for another split within the processing cycle
           */
#  if 0
          g_debug ("skip events scheduled for another split within the processing cycle: ev->time %u, local_offset %u, nframes %u", ev->time, time_nfo->local_offset, time_nfo->nframes);
#  endif
          continue;
        }

#  if 0
      g_message (
        "writing plugin input event %d "
        "at time %u - "
        "local offset %u nframes %u",
        num_events_written,
        ev->time - time_nfo->local_offset,
        time_nfo->local_offset, time_nfo->nframes);
      midi_event_print (ev);
#  endif

      /* event time is relative to the current zrythm full cycle (not split). it
       * needs to be made relative to the current split */
      events[num_events_written].time = ev->time - time_nfo->local_offset;
      events[num_events_written].size = 3;
      events[num_events_written].data[0] = ev->raw_buffer[0];
      events[num_events_written].data[1] = ev->raw_buffer[1];
      events[num_events_written].data[2] = ev->raw_buffer[2];
      num_events_written++;

      if (num_events_written == MAX_EVENTS)
        {
          g_warning ("written %d events", MAX_EVENTS);
          break;
        }
    }
  if (num_events_written > 0)
    {
#  if 0
      engine_process_time_info_print (time_nfo);
      g_debug (
        "Carla plugin %s has %d MIDI events",
        self->plugin->setting->descr->name, num_events_written);
#  endif
    }

  self->native_plugin_descriptor->process (
    self->native_plugin_handle, self->inbufs, self->outbufs, time_nfo->nframes,
    events, (uint32_t) num_events_written);

  /* update latency */
  self->plugin->latency = carla_native_plugin_get_latency (self);
}

static ZPluginCategory
carla_category_to_zrythm_category (CarlaBackend::PluginCategory category)
{
  switch (category)
    {
    case CarlaBackend::PLUGIN_CATEGORY_NONE:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_NONE;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_SYNTH:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_INSTRUMENT;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_DELAY:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_DELAY;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_EQ:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_EQ;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_FILTER:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_FILTER;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_DISTORTION:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_DISTORTION;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_DYNAMICS:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_DYNAMICS;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_MODULATOR:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_MODULATOR;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_UTILITY:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_UTILITY;
      break;
    case CarlaBackend::PLUGIN_CATEGORY_OTHER:
      return ZPluginCategory::Z_PLUGIN_CATEGORY_NONE;
      break;
    }
  g_return_val_if_reached (ZPluginCategory::Z_PLUGIN_CATEGORY_NONE);
}

static char *
carla_category_to_zrythm_category_str (CarlaBackend::PluginCategory category)
{
  switch (category)
    {
    case CarlaBackend::PLUGIN_CATEGORY_SYNTH:
      return g_strdup ("Instrument");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_DELAY:
      return g_strdup ("Delay");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_EQ:
      return g_strdup ("Equalizer");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_FILTER:
      return g_strdup ("Filter");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_DISTORTION:
      return g_strdup ("Distortion");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_DYNAMICS:
      return g_strdup ("Dynamics");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_MODULATOR:
      return g_strdup ("Modulator");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_UTILITY:
      return g_strdup ("Utility");
      break;
    case CarlaBackend::PLUGIN_CATEGORY_OTHER:
    case CarlaBackend::PLUGIN_CATEGORY_NONE:
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
  PluginType                    type)
{
  PluginDescriptor * descr = plugin_descriptor_new ();

  switch (type)
    {
#  if 0
    case CarlalBackend::PLUGIN_INTERNAL:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_CARLA_INTERNAL;
      break;
    case CarlalBackend::PLUGIN_LADSPA:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_LADSPA;
      break;
    case CarlalBackend::PLUGIN_DSSI:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_DSSI;
      break;
#  endif
    case CarlaBackend::PLUGIN_LV2:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2;
      break;
    case CarlaBackend::PLUGIN_VST2:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST;
      break;
#  if 0
    case CarlalBackend::PLUGIN_SF2:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_SF2;
      break;
    case CarlalBackend::PLUGIN_SFZ:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_SFZ;
      break;
    case CarlalBackend::PLUGIN_JACK:
      descr->protocol = ZPluginProtocol::Z_PLUGIN_PROTOCOL_JACK;
      break;
#  endif
    default:
      g_warn_if_reached ();
      break;
    }
  descr->name = g_strdup (info->name);
  descr->author = g_strdup (info->maker);
  descr->num_audio_ins = (int) info->audioIns;
  descr->num_audio_outs = (int) info->audioOuts;
  descr->num_midi_ins = (int) info->midiIns;
  descr->num_midi_outs = (int) info->midiOuts;
  descr->num_ctrl_ins = (int) info->parameterIns;
  descr->num_ctrl_outs = (int) info->parameterOuts;

  descr->category = carla_category_to_zrythm_category (info->category);
  descr->category_str = carla_category_to_zrythm_category_str (info->category);
  descr->min_bridge_mode = plugin_descriptor_get_min_bridge_mode (descr);
  descr->has_custom_ui = info->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI;

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
carla_native_plugin_new_from_setting (Plugin * plugin, GError ** error)
{
  g_return_val_if_fail (plugin->setting->open_with_carla, -1);

  CarlaNativePlugin * self = object_new (CarlaNativePlugin);

  plugin->carla = self;
  self->plugin = plugin;

  return 0;
}

/**
 * Sets the unit of the given control port.
 */
static void
set_unit_from_str (Port * port, const char * unit_str)
{
#  define SET_UNIT(caps, str) \
    if (string_is_equal (unit_str, str)) \
    port->id_.unit_ = PortUnit::Z_PORT_UNIT_##caps

  SET_UNIT (HZ, "Hz");
  SET_UNIT (MS, "ms");
  SET_UNIT (DB, "dB");
  SET_UNIT (SECONDS, "s");
  SET_UNIT (US, "us");

#  undef SET_UNIT
}

static void
create_ports (CarlaNativePlugin * self, bool loading)
{
  g_debug ("%s: loading: %d", __func__, loading);

  char tmp[500];
  char name[4000];

  const PluginDescriptor * descr = self->plugin->setting->descr;
  if (!loading)
    {
      const CarlaPortCountInfo * audio_port_count_nfo =
        carla_get_audio_port_count_info (self->host_handle, 0);
      /* TODO investigate:
       * sometimes port count info returns 1 audio in
       * but the descriptor thinks the plugin has 2 audio
       * ins
       * happens with WAVES Abbey Road Saturator Mono when
       * bridged with yabridge (VST2) */
      z_return_if_fail_cmp (
        (int) audio_port_count_nfo->ins, ==, descr->num_audio_ins);
      int audio_ins_to_create =
        descr->num_audio_ins == 1 ? 2 : descr->num_audio_ins;
      for (int i = 0; i < audio_ins_to_create; i++)
        {
          strcpy (tmp, _ ("Audio in"));
          sprintf (name, "%s %d", tmp, i);
          Port * port = new Port (PortType::Audio, PortFlow::Input, name);
          port->id_.sym_ = fmt::format ("audio_in_{}", i);
#  ifdef CARLA_HAVE_AUDIO_PORT_HINTS
          unsigned int audio_port_hints = carla_get_audio_port_hints (
            self->host_handle, 0, false,
            (uint32_t) (descr->num_audio_ins == 1 ? 0 : i));
          g_debug ("audio port hints %d: %u", i, audio_port_hints);
          if (audio_port_hints & CarlaBackend::AUDIO_PORT_IS_SIDECHAIN)
            {
              g_debug ("%s is sidechain", port->id_.sym_.c_str ());
              port->id_.flags_ |= PortIdentifier::Flags::SIDECHAIN;
            }
#  endif
          plugin_add_in_port (self->plugin, port);
        }

      /* set L/R for sidechain groups (assume
       * stereo) */
      int num_default_sidechains_added = 0;
      for (int i = 0; i < self->plugin->num_in_ports; i++)
        {
          Port * port = self->plugin->in_ports[i];
          if (
            port->id_.type_ == PortType::Audio
            && ENUM_BITSET_TEST (
              PortIdentifier::Flags, port->id_.flags_,
              PortIdentifier::Flags::SIDECHAIN)
            && port->id_.port_group_.empty ()
            && !(ENUM_BITSET_TEST (
              PortIdentifier::Flags, port->id_.flags_,
              PortIdentifier::Flags::STEREO_L))
            && !(ENUM_BITSET_TEST (
              PortIdentifier::Flags, port->id_.flags_,
              PortIdentifier::Flags::STEREO_R)))
            {
              port->id_.port_group_ = ("[Zrythm] Sidechain Group");
              if (num_default_sidechains_added == 0)
                {
                  port->id_.flags_ |= PortIdentifier::Flags::STEREO_L;
                  num_default_sidechains_added++;
                }
              else if (num_default_sidechains_added == 1)
                {
                  port->id_.flags_ |= PortIdentifier::Flags::STEREO_R;
                  break;
                }
            }
        }

      int audio_outs_to_create =
        descr->num_audio_outs == 1 ? 2 : descr->num_audio_outs;
      for (int i = 0; i < audio_outs_to_create; i++)
        {
          strcpy (tmp, _ ("Audio out"));
          sprintf (name, "%s %d", tmp, i);
          Port * port = new Port (PortType::Audio, PortFlow::Output, name);
          port->id_.sym_ = fmt::sprintf ("audio_out_%d", i);
          plugin_add_out_port (self->plugin, port);
        }
      for (int i = 0; i < descr->num_midi_ins; i++)
        {
          strcpy (tmp, _ ("MIDI in"));
          sprintf (name, "%s %d", tmp, i);
          Port * port = new Port (PortType::Event, PortFlow::Input, name);
          port->id_.sym_ = fmt::sprintf ("midi_in_%d", i);
          port->id_.flags2_ |= PortIdentifier::Flags2::SUPPORTS_MIDI;
          plugin_add_in_port (self->plugin, port);
        }
      for (int i = 0; i < descr->num_midi_outs; i++)
        {
          strcpy (tmp, _ ("MIDI out"));
          sprintf (name, "%s %d", tmp, i);
          Port * port = new Port (PortType::Event, PortFlow::Output, name);
          port->id_.sym_ = fmt::sprintf ("midi_out_%d", i);
          port->id_.flags2_ |= PortIdentifier::Flags2::SUPPORTS_MIDI;
          plugin_add_out_port (self->plugin, port);
        }
      for (int i = 0; i < descr->num_cv_ins; i++)
        {
          strcpy (tmp, _ ("CV in"));
          sprintf (name, "%s %d", tmp, i);
          Port * port = new Port (PortType::CV, PortFlow::Input, name);
          port->id_.sym_ = fmt::sprintf ("cv_in_%d", i);
          plugin_add_in_port (self->plugin, port);
        }
      for (int i = 0; i < descr->num_cv_outs; i++)
        {
          strcpy (tmp, _ ("CV out"));
          sprintf (name, "%s %d", tmp, i);
          Port * port = new Port (PortType::CV, PortFlow::Output, name);
          port->id_.sym_ = fmt::sprintf ("cv_out_%d", i);
          plugin_add_out_port (self->plugin, port);
        }
    }

  /* create controls */
  const CarlaPortCountInfo * param_counts =
    carla_get_parameter_count_info (self->host_handle, 0);
#  if 0
  /* FIXME eventually remove this line. this is added
   * because carla discovery reports 0 params for
   * AU plugins, so we update the descriptor here */
  descr->num_ctrl_ins = (int) param_counts->ins;
  g_message (
    "params: %d ins and %d outs",
    descr->num_ctrl_ins, (int) param_counts->outs);
#  endif
  for (uint32_t i = 0; i < param_counts->ins; i++)
    {
      Port * port = NULL;
      if (loading)
        {
          port = carla_native_plugin_get_port_from_param_id (self, i);
          if (!IS_PORT_AND_NONNULL (port))
            {
              const CarlaParameterInfo * param_info =
                carla_get_parameter_info (self->host_handle, 0, i);
              g_warning (
                "port '%s' at param ID %u could not be retrieved [%s], will ignore",
                param_info->name, i, self->plugin->setting->descr->name);
              continue;
            }
        }
      /* else if not loading (create new ports) */
      else
        {
          const CarlaParameterInfo * param_info =
            carla_get_parameter_info (self->host_handle, 0, i);
          port = new Port (PortType::Control, PortFlow::Input, param_info->name);
          if (!IS_PORT_AND_NONNULL (port))
            {
              g_critical (
                "failed to create port '%s' at param ID %u [%s]",
                param_info->name, i, self->plugin->setting->descr->name);
              return;
            }
          if (param_info->symbol && strlen (param_info->symbol) > 0)
            {
              port->id_.sym_ = param_info->symbol;
            }
          else
            {
              port->id_.sym_ = fmt::sprintf ("param_%u", i);
            }
          port->id_.flags_ |= PortIdentifier::Flags::PLUGIN_CONTROL;
          if (param_info->comment && strlen (param_info->comment) > 0)
            {
              port->id_.comment_ = param_info->comment;
            }
          if (param_info->unit && strlen (param_info->unit) > 0)
            {
              set_unit_from_str (port, param_info->unit);
            }
          if (param_info->groupName && strlen (param_info->groupName) > 0)
            {
              char * sub =
                string_get_substr_before_suffix (param_info->groupName, ":");
              g_return_if_fail (sub);
              port->id_.port_group_ = sub;
              g_free (sub);
            }
          port->carla_param_id_ = (int) i;

          const NativeParameter * native_param =
            self->native_plugin_descriptor->get_parameter_info (
              self->native_plugin_handle, i);
          g_return_if_fail (native_param);
          if (native_param->hints & NATIVE_PARAMETER_IS_LOGARITHMIC)
            {
              port->id_.flags_ |= PortIdentifier::Flags::LOGARITHMIC;
            }
          if (native_param->hints & NATIVE_PARAMETER_IS_AUTOMABLE)
            {
              port->id_.flags_ |= PortIdentifier::Flags::AUTOMATABLE;
            }
          if (!(native_param->hints & NATIVE_PARAMETER_IS_ENABLED))
            {
              port->id_.flags_ |= PortIdentifier::Flags::NOT_ON_GUI;
            }
          if (native_param->hints & NATIVE_PARAMETER_IS_BOOLEAN)
            {
              port->id_.flags_ |= PortIdentifier::Flags::TOGGLE;
            }
          else if (native_param->hints & NATIVE_PARAMETER_USES_SCALEPOINTS)
            {
              port->id_.flags2_ |= PortIdentifier::Flags2::ENUMERATION;
            }
          else if (native_param->hints & NATIVE_PARAMETER_IS_INTEGER)
            {
              port->id_.flags_ |= PortIdentifier::Flags::INTEGER;
            }

          /* get scale points */
          if (param_info->scalePointCount > 0)
            {
              port->scale_points_.reserve (param_info->scalePointCount);
            }
          for (uint32_t j = 0; j < param_info->scalePointCount; j++)
            {
              const CarlaScalePointInfo * scale_point_info =
                carla_get_parameter_scalepoint_info (self->host_handle, 0, i, j);

              port->scale_points_.emplace_back (Port::ScalePoint (
                scale_point_info->value, scale_point_info->label));
            }
          std::sort (port->scale_points_.begin (), port->scale_points_.end ());

          plugin_add_in_port (self->plugin, port);

          for (uint32_t j = 0; j < param_info->scalePointCount; j++)
            {
              const CarlaScalePointInfo * sp_info =
                carla_get_parameter_scalepoint_info (self->host_handle, 0, i, j);
              g_debug ("scale point: %s", sp_info->label);
            }

          const ParameterRanges * ranges =
            carla_get_parameter_ranges (self->host_handle, 0, i);
          port->deff_ = ranges->def;
          port->minf_ = ranges->min;
          port->maxf_ = ranges->max;
#  if 0
          g_debug (
            "ranges: min %f max %f default %f",
            (double) port->minf_,
            (double) port->maxf_,
            (double) port->deff_;
#  endif
        }
      float cur_val = carla_native_plugin_get_param_value (self, i);
      g_debug (
        "%d: %s=%f%s", i, port->get_label_as_c_str (), (double) cur_val,
        loading ? " (loading)" : "");
      port->set_control_value (cur_val, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
    }

  self->ports_created = true;
}

static void
patchbay_port_info_free (void * data)
{
  CarlaPatchbayPortInfo * nfo = (CarlaPatchbayPortInfo *) data;
  g_free (nfo->port_name);
  free (nfo);
}

void
carla_native_plugin_update_buffer_size_and_sample_rate (CarlaNativePlugin * self)
{
  g_debug (
    "setting carla buffer size and sample rate: "
    "%u %u",
    AUDIO_ENGINE->block_length, AUDIO_ENGINE->sample_rate);
  carla_set_engine_buffer_size_and_sample_rate (
    self->host_handle, AUDIO_ENGINE->block_length, AUDIO_ENGINE->sample_rate);

  /* update processing buffers */
  unsigned int max_variant_ins =
    self->max_variant_audio_ins + self->max_variant_cv_ins;
  for (size_t i = 0; i < max_variant_ins; i++)
    {
      self->zero_inbufs[i] = static_cast<float *> (g_realloc_n (
        self->zero_inbufs[i], AUDIO_ENGINE->block_length, sizeof (float)));
      dsp_fill (self->zero_inbufs[i], 1e-20f, AUDIO_ENGINE->block_length);
      self->inbufs[i] = self->zero_inbufs[i];
    }
  unsigned int max_variant_outs =
    self->max_variant_audio_outs + self->max_variant_cv_outs;
  for (size_t i = 0; i < max_variant_outs; i++)
    {
      self->zero_outbufs[i] = static_cast<float *> (g_realloc_n (
        self->zero_outbufs[i], AUDIO_ENGINE->block_length, sizeof (float)));
      dsp_fill (self->zero_outbufs[i], 1e-20f, AUDIO_ENGINE->block_length);
      self->outbufs[i] = self->zero_outbufs[i];
    }

  if (self->native_plugin_descriptor->dispatcher)
    {
      intptr_t ret = self->native_plugin_descriptor->dispatcher (
        self->native_plugin_handle, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0,
        AUDIO_ENGINE->block_length, NULL, 0.f);
      g_return_if_fail (ret == 0);
      ret = self->native_plugin_descriptor->dispatcher (
        self->native_plugin_handle, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0,
        0, NULL, (float) AUDIO_ENGINE->sample_rate);
      g_return_if_fail (ret == 0);
    }
  else
    {
      g_warning (
        "native plugin descriptor has no "
        "dispatcher");
    }
}

int
carla_native_plugin_add_internal_plugin_from_descr (
  CarlaNativePlugin *      self,
  const PluginDescriptor * descr)
{
  /** Number of instances to instantiate (1 normally or 2 for mono plugins). */
  int num_instances = descr->num_audio_ins == 1 ? 2 : 1;

  const PluginType type =
    plugin_descriptor_get_carla_plugin_type_from_protocol (descr->protocol);
  int ret = 0;

  for (int i = 0; i < num_instances; i++)
    {
      switch (descr->protocol)
        {
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2:
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_AU:
          g_debug ("uri %s", descr->uri);
          ret = carla_add_plugin (
            self->host_handle,
            descr->arch == ZPluginArchitecture::Z_PLUGIN_ARCHITECTURE_64
              ? CarlaBackend::BINARY_NATIVE
              : CarlaBackend::BINARY_WIN32,
            type, NULL, descr->name, descr->uri, 0, NULL,
            CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST:
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST3:
          ret = carla_add_plugin (
            self->host_handle,
            descr->arch == ZPluginArchitecture::Z_PLUGIN_ARCHITECTURE_64
              ? CarlaBackend::BINARY_NATIVE
              : CarlaBackend::BINARY_WIN32,
            type, descr->path, descr->name, descr->name, descr->unique_id, NULL,
            CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_DSSI:
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LADSPA:
          ret = carla_add_plugin (
            self->host_handle, CarlaBackend::BINARY_NATIVE, type, descr->path,
            descr->name, descr->uri, 0, NULL, CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SFZ:
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_SF2:
          ret = carla_add_plugin (
            self->host_handle, CarlaBackend::BINARY_NATIVE, type, descr->path,
            descr->name, descr->name, 0, NULL,
            CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_JSFX:
          {
            /* the URI is a relative path - make it absolute */
            char * pl_path = plugin_manager_find_plugin_from_rel_path (
              PLUGIN_MANAGER, descr->protocol, descr->uri);
            ret = carla_add_plugin (
              self->host_handle,
              descr->arch == ZPluginArchitecture::Z_PLUGIN_ARCHITECTURE_64
                ? CarlaBackend::BINARY_NATIVE
                : CarlaBackend::BINARY_WIN32,
              type, pl_path, descr->name, descr->name, descr->unique_id, NULL,
              CarlaBackend::PLUGIN_OPTIONS_NULL);
            g_free (pl_path);
          }
          break;
        case ZPluginProtocol::Z_PLUGIN_PROTOCOL_CLAP:
          ret = carla_add_plugin (
            self->host_handle,
            descr->arch == ZPluginArchitecture::Z_PLUGIN_ARCHITECTURE_64
              ? CarlaBackend::BINARY_NATIVE
              : CarlaBackend::BINARY_WIN32,
            type, descr->path, descr->name, descr->uri, 0, NULL,
            CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        default:
          g_return_val_if_reached (-1);
          break;
        }

      if (ret != 1)
        return ret;
    }

  return ret;
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
  self->native_host_descriptor.uiName = g_strdup ("Zrythm");

  self->native_host_descriptor.uiParentId = 0;

  /* set resources dir */
  const char * carla_filename = carla_get_library_filename ();
  char *       tmp = io_get_dir (carla_filename);
  char *       dir = io_get_dir (tmp);
  g_free (tmp);
  tmp = io_get_dir (dir);
  g_free (dir);
  dir = tmp;
  self->native_host_descriptor.resourceDir =
    g_build_filename (dir, "share", "carla", "resources", NULL);
  g_free (dir);

  self->native_host_descriptor.get_buffer_size = host_get_buffer_size;
  self->native_host_descriptor.get_sample_rate = host_get_sample_rate;
  self->native_host_descriptor.is_offline = host_is_offline;
  self->native_host_descriptor.get_time_info = host_get_time_info;
  self->native_host_descriptor.write_midi_event = host_write_midi_event;
  self->native_host_descriptor.ui_parameter_changed = host_ui_parameter_changed;
  self->native_host_descriptor.ui_custom_data_changed =
    host_ui_custom_data_changed;
  self->native_host_descriptor.ui_closed = host_ui_closed;
  self->native_host_descriptor.ui_open_file = NULL;
  self->native_host_descriptor.ui_save_file = NULL;
  self->native_host_descriptor.dispatcher = host_dispatcher;

  self->time_info.bbt.valid = 1;

  /* choose most appropriate patchbay variant */
  self->max_variant_midi_ins = 1;
  self->max_variant_midi_outs = 1;
  const PluginDescriptor * descr = self->plugin->setting->descr;
  if (
    descr->num_audio_ins <= 2 && descr->num_audio_outs <= 2
    && descr->num_cv_ins == 0 && descr->num_cv_outs == 0)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay_plugin ();
      self->max_variant_audio_ins = 2;
      self->max_variant_audio_outs = 2;
      g_message ("using standard patchbay variant");
    }
  else if (
    descr->num_audio_ins <= 16 && descr->num_audio_outs <= 16
    && descr->num_cv_ins == 0 && descr->num_cv_outs == 0)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay16_plugin ();
      self->max_variant_audio_ins = 16;
      self->max_variant_audio_outs = 16;
      g_message ("using patchbay 16 variant");
    }
  else if (
    descr->num_audio_ins <= 32 && descr->num_audio_outs <= 32
    && descr->num_cv_ins == 0 && descr->num_cv_outs == 0)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay32_plugin ();
      self->max_variant_audio_ins = 32;
      self->max_variant_audio_outs = 32;
      g_message ("using patchbay 32 variant");
    }
  else if (
    descr->num_audio_ins <= 64 && descr->num_audio_outs <= 64
    && descr->num_cv_ins == 0 && descr->num_cv_outs == 0)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay64_plugin ();
      self->max_variant_audio_ins = 64;
      self->max_variant_audio_outs = 64;
      g_message ("using patchbay 64 variant");
    }
  else if (
    descr->num_audio_ins <= 2 && descr->num_audio_outs <= 2
    && descr->num_cv_ins <= 5 && descr->num_cv_outs <= 5)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay_cv_plugin ();
      self->max_variant_audio_ins = 2;
      self->max_variant_audio_outs = 2;
      self->max_variant_cv_ins = 5;
      self->max_variant_cv_outs = 5;
      g_message ("using patchbay CV variant");
    }
#  ifdef CARLA_HAVE_CV8_PATCHBAY_VARIANT
  else if (
    descr->num_audio_ins <= 2 && descr->num_audio_outs <= 2
    && descr->num_cv_ins <= 8 && descr->num_cv_outs <= 8)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay_cv8_plugin ();
      self->max_variant_audio_ins = 2;
      self->max_variant_audio_outs = 2;
      self->max_variant_cv_ins = 8;
      self->max_variant_cv_outs = 8;
      g_message ("using patchbay CV8 variant");
    }
#  endif
#  ifdef CARLA_HAVE_CV32_PATCHBAY_VARIANT
  else if (
    descr->num_audio_ins <= 64 && descr->num_audio_outs <= 64
    && descr->num_cv_ins <= 32 && descr->num_cv_outs <= 32)
    {
      self->native_plugin_descriptor = carla_get_native_patchbay_cv32_plugin ();
      self->max_variant_audio_ins = 64;
      self->max_variant_audio_outs = 64;
      self->max_variant_cv_ins = 32;
      self->max_variant_cv_outs = 32;
      g_message ("using patchbay CV32 variant");
    }
#  endif
  else
    {
      g_warning (
        "Plugin with %d audio ins, %d audio outs, "
        "%d CV ins, %d CV outs not fully "
        "supported. Using standard Carla Patchbay "
        "variant",
        descr->num_audio_ins, descr->num_audio_outs, descr->num_cv_ins,
        descr->num_cv_outs);
      self->native_plugin_descriptor = carla_get_native_patchbay_plugin ();
      self->max_variant_audio_ins = 2;
      self->max_variant_audio_outs = 2;
    }

  unsigned int max_variant_ins =
    self->max_variant_audio_ins + self->max_variant_cv_ins;
  self->zero_inbufs = object_new_n (max_variant_ins, float *);
  self->inbufs = object_new_n (max_variant_ins, float *);
  for (size_t i = 0; i < max_variant_ins; i++)
    {
      self->zero_inbufs[i] = object_new_n (AUDIO_ENGINE->block_length, float);
    }
  unsigned int max_variant_outs =
    self->max_variant_audio_outs + self->max_variant_cv_outs;
  self->zero_outbufs = object_new_n (max_variant_outs, float *);
  self->outbufs = object_new_n (max_variant_ins, float *);
  for (size_t i = 0; i < max_variant_outs; i++)
    {
      self->zero_outbufs[i] = object_new_n (AUDIO_ENGINE->block_length, float);
    }

  /* instantiate the plugin to get its info */
  self->native_plugin_handle =
    self->native_plugin_descriptor->instantiate (&self->native_host_descriptor);
  if (!self->native_plugin_handle)
    {
      g_set_error_literal (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
        "Failed to get native plugin handle");
      return -1;
    }
  self->host_handle = carla_create_native_plugin_host_handle (
    self->native_plugin_descriptor, self->native_plugin_handle);
  if (!self->host_handle)
    {
      g_set_error_literal (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
        "Failed to get host handle");
      return -1;
    }
  self->carla_plugin_id = 0;

  /* set binary paths */
  char * zrythm_libdir =
    gZrythm->dir_mgr->get_dir (ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR);
  char * carla_binaries_dir = g_build_filename (zrythm_libdir, "carla", NULL);
  g_message (
    "setting carla engine option [ENGINE_OPTION_PATH_BINARIES] to '%s'",
    carla_binaries_dir);
  carla_set_engine_option (
    self->host_handle, CarlaBackend::ENGINE_OPTION_PATH_BINARIES, 0,
    carla_binaries_dir);
  g_free (zrythm_libdir);
  g_free (carla_binaries_dir);

  /* set plugin paths */
  {
    char * paths = plugin_manager_get_paths_for_protocol_separated (
      PLUGIN_MANAGER, descr->protocol);
    PluginType ptype =
      plugin_descriptor_get_carla_plugin_type_from_protocol (descr->protocol);
    carla_set_engine_option (
      self->host_handle, CarlaBackend::ENGINE_OPTION_PLUGIN_PATH, ptype, paths);
    g_free (paths);
  }

  /* set UI scale factor */
  carla_set_engine_option (
    self->host_handle, CarlaBackend::ENGINE_OPTION_FRONTEND_UI_SCALE,
    (int) ((float) self->plugin->ui_scale_factor * 1000.f), NULL);

  /* set whether UI should stay on top */
  /* disable for now */
#  if 0
  if (!ZRYTHM_TESTING &&
      g_settings_get_boolean (
        S_P_PLUGINS_UIS, "stay-on-top"))
    {
      carla_set_engine_option (
        self->host_handle,
        CarlaBackend::ENGINE_OPTION_UIS_ALWAYS_ON_TOP, true,
        NULL);
    }
#  endif

  carla_native_plugin_update_buffer_size_and_sample_rate (self);

  const PluginSetting * setting = self->plugin->setting;
  g_return_val_if_fail (setting->open_with_carla, -1);
  g_message (
    "%s: using bridge mode %s", __func__, ENUM_NAME (setting->bridge_mode));

  /* set bridging on if needed */
  switch (setting->bridge_mode)
    {
    case CarlaBridgeMode::Full:
      g_message ("plugin must be bridged whole, using plugin bridge");
      carla_set_engine_option (
        self->host_handle, CarlaBackend::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES,
        true, NULL);
      break;
    case CarlaBridgeMode::UI:
      g_message ("using UI bridge only");
      carla_set_engine_option (
        self->host_handle, CarlaBackend::ENGINE_OPTION_PREFER_UI_BRIDGES, true,
        NULL);
      break;
    default:
      break;
    }

  /* raise bridge timeout to 8 sec */
  if (
    setting->bridge_mode == CarlaBridgeMode::Full
    || setting->bridge_mode == CarlaBridgeMode::UI)
    {
      carla_set_engine_option (
        self->host_handle, CarlaBackend::ENGINE_OPTION_UI_BRIDGES_TIMEOUT, 8000,
        NULL);
    }

  intptr_t uses_embed_ret = self->native_plugin_descriptor->dispatcher (
    self->native_plugin_handle, NATIVE_PLUGIN_OPCODE_HOST_USES_EMBED, 0, 0,
    NULL, 0.f);
  if (uses_embed_ret != 0)
    {
      g_set_error_literal (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
        "Failed to set USES_EMBED");
      return -1;
    }

  int ret = carla_native_plugin_add_internal_plugin_from_descr (self, descr);

  carla_native_plugin_update_buffer_size_and_sample_rate (self);

  if (ret != 1)
    {
      g_set_error (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED,
        _ ("Error adding carla plugin: %s"),
        carla_get_last_error (self->host_handle));
      return -1;
    }

    /* enable various messages */
#  define ENABLE_OPTION(x) \
    carla_set_option ( \
      self->host_handle, 0, CarlaBackend::PLUGIN_OPTION_##x, true)

  ENABLE_OPTION (SEND_CONTROL_CHANGES);
  ENABLE_OPTION (SEND_CHANNEL_PRESSURE);
  ENABLE_OPTION (SEND_NOTE_AFTERTOUCH);
  ENABLE_OPTION (SEND_PITCHBEND);
  ENABLE_OPTION (SEND_ALL_SOUND_OFF);
  ENABLE_OPTION (SEND_PROGRAM_CHANGES);

#  undef ENABLE_OPTION

  /* add engine callback */
  carla_set_engine_callback (self->host_handle, carla_engine_callback, self);

  self->patchbay_port_info =
    g_ptr_array_new_with_free_func (patchbay_port_info_free);

  g_debug ("refreshing patchbay");
  carla_patchbay_refresh (self->host_handle, false);
  g_debug ("refreshed patchbay");

  /* connect to patchbay ports */
  unsigned int num_audio_ins_connected = 0;
  unsigned int num_audio_outs_connected = 0;
  unsigned int num_cv_ins_connected = 0;
  unsigned int num_cv_outs_connected = 0;
  unsigned int num_midi_ins_connected = 0;
  unsigned int num_midi_outs_connected = 0;
  bool         is_cv_variant =
    self->max_variant_cv_ins > 0 || self->max_variant_cv_outs > 0;
  for (size_t i = 0; i < self->patchbay_port_info->len; i++)
    {
      CarlaPatchbayPortInfo * nfo = (CarlaPatchbayPortInfo *)
        g_ptr_array_index (self->patchbay_port_info, i);
      unsigned int plugin_id = nfo->plugin_id;
      unsigned int port_hints = nfo->port_hints;
      unsigned int port_id = nfo->port_id;
      char *       port_name = nfo->port_name;
      g_debug (
        "processing %s, plugin id %u, "
        "portid %u, port hints %u",
        port_name, plugin_id, port_id, port_hints);
      if (port_hints & CarlaBackend::PATCHBAY_PORT_IS_INPUT)
        {
          if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_AUDIO
            && num_audio_ins_connected < self->max_variant_audio_ins)
            {
              g_debug (
                "connecting %d:%u to %d:%u", 1,
                self->audio_input_port_id + num_audio_ins_connected, plugin_id,
                port_id);
              ret = carla_patchbay_connect (
                self->host_handle, false, 1,
                self->audio_input_port_id + num_audio_ins_connected++,
                plugin_id, port_id);
              if (!ret)
                {
                  g_critical (
                    "Error: %s", carla_get_last_error (self->host_handle));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_CV
            && num_audio_ins_connected < self->max_variant_cv_ins)
            {
              g_debug (
                "connecting %d:%u to %d:%u", 3,
                self->cv_input_port_id + num_cv_ins_connected, plugin_id,
                port_id);
              ret = carla_patchbay_connect (
                self->host_handle, false, 3,
                self->cv_input_port_id + num_cv_ins_connected++, plugin_id,
                port_id);
              if (!ret)
                {
                  g_critical (
                    "Error: %s", carla_get_last_error (self->host_handle));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_MIDI
            && num_midi_ins_connected < self->max_variant_midi_ins)
            {
              g_debug (
                "connecting %d:%u to %d:%u", is_cv_variant ? 5 : 3,
                self->midi_input_port_id + num_midi_ins_connected, plugin_id,
                port_id);
              ret = carla_patchbay_connect (
                self->host_handle, false, is_cv_variant ? 5 : 3,
                self->midi_input_port_id + num_midi_ins_connected++, plugin_id,
                port_id);
              if (!ret)
                {
                  g_critical (
                    "Error: %s", carla_get_last_error (self->host_handle));
                }
            }
        }
      /* else if output */
      else
        {
          if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_AUDIO
            && num_audio_outs_connected < self->max_variant_audio_outs)
            {
              g_debug (
                "connecting %d:%u to %d:%u", plugin_id, port_id, 2,
                self->audio_output_port_id + num_audio_outs_connected);
              ret = carla_patchbay_connect (
                self->host_handle, false, plugin_id, port_id, 2,
                self->audio_output_port_id + num_audio_outs_connected++);
              if (!ret)
                {
                  g_critical (
                    "Error: %s", carla_get_last_error (self->host_handle));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_CV
            && num_cv_outs_connected < self->max_variant_cv_outs)
            {
              g_debug (
                "connecting %d:%u to %d:%u", plugin_id, port_id, 4,
                self->cv_output_port_id + num_cv_outs_connected);
              ret = carla_patchbay_connect (
                self->host_handle, false, plugin_id, port_id, 4,
                self->cv_output_port_id + num_cv_outs_connected++);
              if (!ret)
                {
                  g_critical (
                    "Error: %s", carla_get_last_error (self->host_handle));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_MIDI
            && num_midi_outs_connected < self->max_variant_midi_outs)
            {
              g_debug (
                "connecting %d:%u to %d:%u", plugin_id, port_id,
                is_cv_variant ? 6 : 4,
                self->midi_output_port_id + num_midi_outs_connected);
              ret = carla_patchbay_connect (
                self->host_handle, false, plugin_id, port_id,
                is_cv_variant ? 6 : 4,
                self->midi_output_port_id + num_midi_outs_connected++);
              if (!ret)
                {
                  g_warning (
                    "Error: %s", carla_get_last_error (self->host_handle));
                }
            }
        }
    }

  if (use_state_file)
    {
      /* load the state */
      GError * err = NULL;
      bool     state_loaded =
        carla_native_plugin_load_state (self->plugin->carla, NULL, &err);
      if (!state_loaded)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to load Carla state"));
        }
    }

  if (
    !self->native_plugin_handle || !self->native_plugin_descriptor->activate
    || !self->native_plugin_descriptor->ui_show)
    {
      g_set_error (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_INSTANTIATION_FAILED, "%s",
        _ ("Failed to instantiate Carla plugin: handle/descriptor not initialized properly"));
      self->plugin->instantiation_failed = true;
      return -1;
    }

  /* create ports */
  if (!loading && !use_state_file && !self->ports_created)
    {
      create_ports (self, false);
    }

  g_message ("activating carla plugin...");
  self->native_plugin_descriptor->activate (self->native_plugin_handle);
  g_message ("carla plugin activated");

  carla_native_plugin_update_buffer_size_and_sample_rate (self);

  /* load data into existing ports */
  if (loading || use_state_file)
    {
      create_ports (self, true);
    }

  return 0;
}

static void
carla_plugin_tick_cb_destroy (void * data)
{
  g_return_if_fail (data);
  CarlaNativePlugin * self = (CarlaNativePlugin *) data;

  self->tick_cb = 0;
}

/**
 * Shows or hides the UI.
 */
void
carla_native_plugin_open_ui (CarlaNativePlugin * self, bool show)
{
  Plugin * pl = self->plugin;
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));
  g_return_if_fail (plugin_is_in_active_project (pl));
  g_return_if_fail (pl->setting->descr);

  g_message (
    "%s: show/hide '%s (%p)' UI: %d", __func__, pl->setting->descr->name, pl,
    show);

  if ((self->tick_cb == 0 && !show) || (self->tick_cb != 0 && show))
    {
      g_message (
        "plugin already has visibility status %d, "
        "doing nothing",
        show);
      return;
    }

  switch (pl->setting->descr->protocol)
    {
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_VST3:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_DSSI:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_LV2:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_AU:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_CLAP:
    case ZPluginProtocol::Z_PLUGIN_PROTOCOL_JSFX:
      {
        if (show)
          {
            char * title = plugin_generate_window_title (pl);
            g_debug ("plugin window title '%s'", title);
            carla_set_custom_ui_title (self->host_handle, 0, title);
            g_free (title);

            /* set whether to keep window on top */
            if (
              ZRYTHM_HAVE_UI
              && g_settings_get_boolean (S_P_PLUGINS_UIS, "stay-on-top"))
              {
#  if defined(HAVE_X11) && !defined(GDK_WINDOWING_WAYLAND)
                Window xid = z_gtk_window_get_x11_xid (GTK_WINDOW (MAIN_WINDOW));
                g_debug (
                  "FRONTEND_WIN_ID: "
                  "setting X11 parent to %lx",
                  xid);
                char xid_str[400];
                sprintf (xid_str, "%lx", xid);
                carla_set_engine_option (
                  self->host_handle,
                  CarlaBackend::ENGINE_OPTION_FRONTEND_WIN_ID, 0, xid_str);
#  else
                g_warning (
                  "stay-on-top unavailable on this "
                  "window manager");
#  endif
              }
          }

#  if defined(_WIN32)
        HWND hwnd = z_gtk_window_get_windows_hwnd (GTK_WINDOW (MAIN_WINDOW));
        g_debug ("FRONTEND_WIN_ID: setting Windows parent to %" PRIxPTR, hwnd);
        char hwnd_str[400];
        sprintf (hwnd_str, "%" PRIxPTR, hwnd);
        carla_set_engine_option (
          self->host_handle, CarlaBackend::ENGINE_OPTION_FRONTEND_WIN_ID, 0,
          hwnd_str);
#  endif

        {
          g_message (
            "Attempting to %s UI for %s", show ? "show" : "hide",
            pl->setting->descr->name);
          GdkGLContext * context = clear_gl_context ();
          carla_show_custom_ui (self->host_handle, 0, show);
          return_gl_context (context);
          g_message ("Completed %s UI", show ? "show" : "hide");
          pl->visible = show;
        }

        if (self->tick_cb)
          {
            g_debug ("removing tick callback for %s", pl->setting->descr->name);
            g_source_remove (self->tick_cb);
            self->tick_cb = 0;
          }

        if (show)
          {
            g_return_if_fail (MAIN_WINDOW);
            g_debug ("setting tick callback for %s", pl->setting->descr->name);
            /* do not use tick callback: */
            /* falktx: I am doing some checks on ildaeil/carla, and see there is
             * a nice way without conflicts to avoid the GL context issues. it
             * came from cardinal, where I cannot draw plugin UIs in the same
             * function as the main stuff, because it is in between other opengl
             * calls (before and after). the solution I found was to have a
             * dedicated idle timer, and handle the plugin UI stuff there,
             * outside of the main application draw function */
            self->tick_cb = g_timeout_add_full (
              G_PRIORITY_DEFAULT,
              /* 60 fps */
              1000 / 60, carla_plugin_tick_cb, self,
              carla_plugin_tick_cb_destroy);
          }

        if (!ZRYTHM_TESTING)
          {
            EVENTS_PUSH (EventType::ET_PLUGIN_WINDOW_VISIBILITY_CHANGED, pl);
          }
      }
      break;
    default:
      break;
    }
}

int
carla_native_plugin_activate (CarlaNativePlugin * self, bool activate)
{
  g_message (
    "setting plugin %s active %d", self->plugin->setting->descr->name, activate);
  carla_set_active (self->host_handle, 0, activate);

  return 0;
}

float
carla_native_plugin_get_param_value (CarlaNativePlugin * self, const uint32_t id)
{
  return carla_get_current_parameter_value (self->host_handle, 0, id);
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

  float cur_val = carla_get_current_parameter_value (self->host_handle, 0, id);
  if (DEBUGGING && !math_floats_equal (cur_val, val))
    {
      g_debug ("setting param %d value to %f", id, (double) val);
    }
  carla_set_parameter_value (self->host_handle, 0, id, val);
  if (carla_get_current_plugin_count (self->host_handle) == 2)
    {
      carla_set_parameter_value (self->host_handle, 1, id, val);
    }
}

/**
 * Returns the MIDI out port.
 */
Port *
carla_native_plugin_get_midi_out_port (CarlaNativePlugin * self)
{
  Plugin * pl = self->plugin;
  Port *   port;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->out_ports[i];
      if (
        port->id_.type_ == PortType::Event
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, port->id_.flags2_,
          PortIdentifier::Flags2::SUPPORTS_MIDI))
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
  Port *   port;
  int      j = 0;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];
      if (port->id_.type_ != PortType::Control)
        continue;

      j = port->carla_param_id_;
      if ((int) id == port->carla_param_id_)
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
 * Returns the latency in samples.
 */
nframes_t
carla_native_plugin_get_latency (CarlaNativePlugin * self)
{
  return carla_get_plugin_latency (self->host_handle, 0);
}

/**
 * Saves the state inside the standard state directory.
 *
 * @param is_backup Whether this is a backup project. Used for
 *   calculating the absolute path to the state dir.
 * @param abs_state_dir If passed, the state will be saved
 *   inside this directory instead of the plugin's state
 *   directory. Used when saving presets.
 *
 * @return Whether successful.
 */
bool
carla_native_plugin_save_state (
  CarlaNativePlugin * self,
  bool                is_backup,
  const char *        abs_state_dir,
  GError **           error)
{
  if (!self->plugin->instantiated)
    {
      g_debug (
        "plugin %s not instantiated, skipping %s",
        self->plugin->setting->descr->name, __func__);
      return true;
    }

  char * dir_to_use = NULL;
  if (abs_state_dir)
    {
      dir_to_use = g_strdup (abs_state_dir);
    }
  else
    {
      dir_to_use = plugin_get_abs_state_dir (self->plugin, is_backup, true);
    }
  GError * err = NULL;
  bool     success = io_mkdir (dir_to_use, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR_LITERAL (
        error, err, "Failed to create backup directory");
      return false;
    }
  char * state_file_abs_path =
    g_build_filename (dir_to_use, CARLA_STATE_FILENAME, NULL);
  g_debug ("saving carla plugin state to %s", state_file_abs_path);
  bool ret = carla_save_plugin_state (self->host_handle, 0, state_file_abs_path);
  if (ret != true)
    {
      g_warning (
        "failed to save plugin state: %s",
        carla_get_last_error (self->host_handle));
    }
  g_free (state_file_abs_path);

  g_warn_if_fail (self->plugin->state_dir);

  return true;
}

#  if 0
char *
carla_native_plugin_get_abs_state_file_path (
  const CarlaNativePlugin * self,
  const bool                is_backup)
{
  char * abs_state_dir =
    plugin_get_abs_state_dir (self->plugin, is_backup);
  char * state_file_abs_path = g_build_filename (
    abs_state_dir, CARLA_STATE_FILENAME, NULL);

  g_free (abs_state_dir);

  return state_file_abs_path;
}
#  endif

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
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  char * state_file;
  if (abs_path)
    {
      state_file = g_strdup (abs_path);
    }
  else
    {
      if (!pl->state_dir)
        {
          g_set_error_literal (
            error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
            Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_FAILED,
            _ ("Plugin doesn't have a state directory"));
          return false;
        }
      char * state_dir_abs_path =
        plugin_get_abs_state_dir (pl, PROJECT->loading_from_backup, false);
      state_file =
        g_build_filename (state_dir_abs_path, CARLA_STATE_FILENAME, NULL);
      g_free (state_dir_abs_path);
    }
  g_debug ("loading state from %s (given path: %s)...", state_file, abs_path);

  if (!file_exists (state_file))
    {
      g_set_error (
        error, Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR,
        Z_PLUGINS_CARLA_NATIVE_PLUGIN_ERROR_FAILED,
        _ ("State file %s doesn't exist"), state_file);
      return false;
    }

  self->loading_state = true;
  carla_load_plugin_state (self->host_handle, 0, state_file);
  uint32_t plugin_count = carla_get_current_plugin_count (self->host_handle);
  if (plugin_count == 2)
    {
      carla_load_plugin_state (self->host_handle, 1, state_file);
    }
  self->loading_state = false;
  if (pl->visible && plugin_is_in_active_project (pl))
    {
      EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, pl);
    }
  g_message (
    "%s: successfully loaded carla plugin state "
    "from %s",
    __func__, state_file);

  g_free (state_file);

  return true;
}

void
carla_native_plugin_close (CarlaNativePlugin * self)
{
  if (self->host_handle)
    {
      g_debug ("setting carla engine about to close...");
      carla_set_engine_about_to_close (self->host_handle);
    }

  if (self->tick_cb)
    {
      g_source_remove (self->tick_cb);
      self->tick_cb = 0;
    }

  PluginDescriptor * descr = self->plugin->setting->descr;
  const char *       name = descr->name;
  g_debug ("closing plugin %s...", name);
  if (self->native_plugin_descriptor)
    {
      g_debug ("deactivating %s...", name);
      self->native_plugin_descriptor->deactivate (self->native_plugin_handle);
      g_debug ("deactivated %s", name);
      g_debug ("cleaning up %s...", name);
      self->native_plugin_descriptor->cleanup (self->native_plugin_handle);
      g_debug ("cleaned up %s", name);
      self->native_plugin_descriptor = NULL;
    }
  if (self->host_handle)
    {
      g_debug ("freeing host handle for %s...", name);
      carla_host_handle_free (self->host_handle);
      self->host_handle = NULL;
      g_debug ("free'd host handle for %s", name);
    }
  g_debug ("closed plugin %s", name);
}

/**
 * Deactivates, cleanups and frees the instance.
 */
void
carla_native_plugin_free (CarlaNativePlugin * self)
{
  carla_native_plugin_close (self);

  unsigned int max_variant_ins =
    self->max_variant_audio_ins + self->max_variant_cv_ins;
  for (size_t i = 0; i < max_variant_ins; i++)
    {
      object_free_w_func_and_null (free, self->zero_inbufs[i]);
    }
  object_free_w_func_and_null (free, self->zero_inbufs);
  object_free_w_func_and_null (free, self->inbufs);
  unsigned int max_variant_outs =
    self->max_variant_audio_outs + self->max_variant_cv_outs;
  for (size_t i = 0; i < max_variant_outs; i++)
    {
      object_free_w_func_and_null (free, self->zero_outbufs[i]);
    }
  object_free_w_func_and_null (free, self->zero_outbufs);
  object_free_w_func_and_null (free, self->outbufs);

  object_free_w_func_and_null (g_ptr_array_unref, self->patchbay_port_info);

  object_zero_and_free (self);
}
#endif
