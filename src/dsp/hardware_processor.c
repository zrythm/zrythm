// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/ext_port.h"
#include "dsp/hardware_processor.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
hardware_processor_init_loaded (HardwareProcessor * self, AudioEngine * engine)
{
  self->engine = engine;
  self->ext_audio_ports_size = (size_t) self->num_ext_audio_ports;
  self->ext_midi_ports_size = (size_t) self->num_ext_midi_ports;
}

/**
 * Returns a new empty instance.
 */
HardwareProcessor *
hardware_processor_new (bool input, AudioEngine * engine)
{
  HardwareProcessor * self = object_new (HardwareProcessor);
  self->schema_version = HW_PROCESSOR_SCHEMA_VERSION;
  self->engine = engine;
  self->is_input = input;

  return self;
}

/**
 * Finds an ext port from its ID (type + full name).
 *
 * @see ext_port_get_id()
 */
ExtPort *
hardware_processor_find_ext_port (HardwareProcessor * self, const char * id)
{
  g_return_val_if_fail (self && id, NULL);

  for (int i = 0; i < self->num_ext_audio_ports; i++)
    {
      ExtPort * port = self->ext_audio_ports[i];
      char *    port_id = ext_port_get_id (port);
      if (string_is_equal (port_id, id))
        {
          g_free (port_id);
          return port;
        }
      g_free (port_id);
    }
  for (int i = 0; i < self->num_ext_midi_ports; i++)
    {
      ExtPort * port = self->ext_midi_ports[i];
      char *    port_id = ext_port_get_id (port);
      if (string_is_equal (port_id, id))
        {
          g_free (port_id);
          return port;
        }
      g_free (port_id);
    }

  return NULL;
}

/**
 * Finds a port from its ID (type + full name).
 *
 * @see ext_port_get_id()
 */
Port *
hardware_processor_find_port (HardwareProcessor * self, const char * id)
{
  g_return_val_if_fail (self && id, NULL);

  for (int i = 0; i < self->num_ext_audio_ports; i++)
    {
      ExtPort * ext_port = self->ext_audio_ports[i];
      char *    ext_port_id = ext_port_get_id (ext_port);
      Port *    port = self->audio_ports[i];
      if (!PROJECT || !PROJECT->loaded)
        {
          port->magic = PORT_MAGIC;
        }
      g_return_val_if_fail (ext_port && IS_PORT (port), NULL);
      if (string_is_equal (ext_port_id, id))
        {
          g_free (ext_port_id);
          return port;
        }
      g_free (ext_port_id);
    }
  for (int i = 0; i < self->num_ext_midi_ports; i++)
    {
      ExtPort * ext_port = self->ext_midi_ports[i];
      char *    ext_port_id = ext_port_get_id (ext_port);
      Port *    port = self->midi_ports[i];
      if (!PROJECT || !PROJECT->loaded)
        {
          port->magic = PORT_MAGIC;
        }
      g_return_val_if_fail (ext_port && IS_PORT (port), NULL);
      if (string_is_equal (ext_port_id, id))
        {
          g_free (ext_port_id);
          return port;
        }
      g_free (ext_port_id);
    }

  return NULL;
}

static Port *
create_port_for_ext_port (ExtPort * ext_port, PortType type, PortFlow flow)
{
  Port * port = port_new_with_type_and_owner (
    type, flow, ext_port->full_name, PORT_OWNER_TYPE_HW, ext_port);
  port->id.flags |= PORT_FLAG_HW;
  port->id.ext_port_id = ext_port_get_id (ext_port);

  return port;
}

/**
 * Rescans the hardware ports and appends any missing
 * ones.
 *
 * @note This is a GSource callback.
 */
bool
hardware_processor_rescan_ext_ports (HardwareProcessor * self)
{
  g_debug ("rescanning ports...");

  /* get correct flow */
  PortFlow flow =
    /* these are reversed:
     * input here -> port that outputs in backend */
    self->is_input ? FLOW_OUTPUT : FLOW_INPUT;

  /* collect audio ports */
  GPtrArray * ports = g_ptr_array_new ();
  ext_ports_get (TYPE_AUDIO, flow, true, ports);

  /* add missing ports to the list */
  for (size_t i = 0; i < ports->len; i++)
    {
      ExtPort * ext_port = g_ptr_array_index (ports, i);
      ExtPort * existing_port =
        hardware_processor_find_ext_port (self, ext_port_get_id (ext_port));

      if (!existing_port)
        {
          array_double_size_if_full (
            self->ext_audio_ports, self->num_ext_audio_ports,
            self->ext_audio_ports_size, ExtPort *);
          ext_port->hw_processor = self;
          array_append (
            self->ext_audio_ports, self->num_ext_audio_ports, ext_port);
          char * id = ext_port_get_id (ext_port);
          g_message ("[HW] Added audio port %s", id);
          g_free (id);
        }
      else
        {
          ext_port_free (ext_port);
        }
    }
  g_ptr_array_unref (ports);

  /* collect midi ports */
  ports = g_ptr_array_new ();
  ext_ports_get (TYPE_EVENT, flow, true, ports);

  /* add missing ports to the list */
  for (size_t i = 0; i < ports->len; i++)
    {
      ExtPort * ext_port = g_ptr_array_index (ports, i);
      ExtPort * existing_port =
        hardware_processor_find_ext_port (self, ext_port_get_id (ext_port));

      if (!existing_port)
        {
          array_double_size_if_full (
            self->ext_midi_ports, self->num_ext_midi_ports,
            self->ext_midi_ports_size, ExtPort *);
          ext_port->hw_processor = self;
          array_append (
            self->ext_midi_ports, self->num_ext_midi_ports, ext_port);
          char * id = ext_port_get_id (ext_port);
          g_message ("[HW] Added MIDI port %s", id);
          g_free (id);
        }
      else
        {
          ext_port_free (ext_port);
        }
    }
  g_ptr_array_unref (ports);

  /* create ports for each ext port */
  self->audio_ports = g_realloc (
    self->audio_ports, MAX (1, self->ext_audio_ports_size) * sizeof (Port *));
  self->midi_ports = g_realloc (
    self->midi_ports, MAX (1, self->ext_midi_ports_size) * sizeof (Port *));
  for (int i = 0; i < self->num_ext_audio_ports; i++)
    {
      ExtPort * ext_port = self->ext_audio_ports[i];
      g_return_val_if_fail (ext_port, G_SOURCE_REMOVE);
      if (i >= self->num_audio_ports)
        {
          g_return_val_if_fail (self->audio_ports, G_SOURCE_REMOVE);
          self->audio_ports[i] =
            create_port_for_ext_port (ext_port, TYPE_AUDIO, FLOW_OUTPUT);
          self->num_audio_ports++;
        }

      g_return_val_if_fail (IS_PORT (self->audio_ports[i]), G_SOURCE_REMOVE);
    }
  for (int i = 0; i < self->num_ext_midi_ports; i++)
    {
      ExtPort * ext_port = self->ext_midi_ports[i];
      g_return_val_if_fail (ext_port, G_SOURCE_REMOVE);
      if (i >= self->num_midi_ports)
        {
          self->midi_ports[i] =
            create_port_for_ext_port (ext_port, TYPE_EVENT, FLOW_OUTPUT);
          self->num_midi_ports++;
        }

      g_return_val_if_fail (IS_PORT (self->midi_ports[i]), G_SOURCE_REMOVE);
    }

  /* TODO deactivate ports that weren't found
   * (stop engine temporarily to remove) */

  g_debug (
    "[%s] have %d audio and %d MIDI ports",
    self->is_input ? "HW processor inputs" : "HW processor outputs",
    self->num_ext_audio_ports, self->num_ext_midi_ports);

  for (int i = 0; i < self->num_ext_audio_ports; i++)
    {
      ExtPort * ext_port = self->ext_audio_ports[i];
      char *    id = ext_port_get_id (ext_port);
#if 0
      g_debug (
        "[%s] audio: %s",
        self->is_input ? "HW processor input" : "HW processor output", id);
#endif
      g_free (id);
    }
  for (int i = 0; i < self->num_ext_midi_ports; i++)
    {
      ExtPort * ext_port = self->ext_midi_ports[i];
      char *    id = ext_port_get_id (ext_port);
#if 0
      g_debug (
        "[%s] MIDI: %s",
        self->is_input ? "HW processor input" : "HW processor output", id);
#endif
      g_free (id);

      /* attempt to reconnect the if the port
       * needs reconnect (e.g. if disconnected
       * earlier) */
      if (ext_port->pending_reconnect)
        {
          Port * port = self->midi_ports[i];
          int    ret = ext_port_activate (ext_port, port, true);
          if (ret == 0)
            {
              ext_port->pending_reconnect = false;
            }
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Sets up the ports but does not start them.
 *
 * @return Non-zero on fail.
 */
int
hardware_processor_setup (HardwareProcessor * self)
{
  if (ZRYTHM_TESTING || ZRYTHM_GENERATING_PROJECT)
    return 0;

  g_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD && S_P_GENERAL_ENGINE, -1);

  if (self->is_input)
    {
      /* cache selections */
      self->selected_midi_ports =
        g_settings_get_strv (S_P_GENERAL_ENGINE, "midi-controllers");
      self->selected_audio_ports =
        g_settings_get_strv (S_P_GENERAL_ENGINE, "audio-inputs");

      /* get counts */
      self->num_selected_midi_ports = 0;
      self->num_selected_audio_ports = 0;
      while ((self->selected_midi_ports[self->num_selected_midi_ports++]))
        ;
      self->num_selected_midi_ports--;
      while ((self->selected_audio_ports[self->num_selected_audio_ports++]))
        ;
      self->num_selected_audio_ports--;
    }

  /* ---- scan current ports ---- */

  hardware_processor_rescan_ext_ports (self);

  /* ---- end scan ---- */

  self->setup = true;

  return 0;
}

/**
 * Starts or stops the ports.
 *
 * @param activate True to activate, false to
 *   deactivate
 */
void
hardware_processor_activate (HardwareProcessor * self, bool activate)
{
  g_message ("hw processor activate: %d", activate);

  /* go through each selected port and activate/
   * deactivate */
  for (int i = 0; i < self->num_selected_midi_ports; i++)
    {
      char *    selected_port = self->selected_midi_ports[i];
      ExtPort * ext_port =
        hardware_processor_find_ext_port (self, selected_port);
      Port * port = hardware_processor_find_port (self, selected_port);
      if (port && ext_port)
        {
          ext_port_activate (ext_port, port, activate);
        }
      else
        {
          g_message ("could not find port %s", selected_port);
        }
    }
  for (int i = 0; i < self->num_selected_audio_ports; i++)
    {
      char *    selected_port = self->selected_audio_ports[i];
      ExtPort * ext_port =
        hardware_processor_find_ext_port (self, selected_port);
      Port * port = hardware_processor_find_port (self, selected_port);
      if (port && ext_port)
        {
          ext_port_activate (ext_port, port, activate);
        }
      else
        {
          g_message ("could not find port %s", selected_port);
        }
    }

  if (activate && !self->rescan_timeout_id)
    {
      /* TODO: do the following on another thread - this blocks the UI
       * until then, the rescan is done when temporarily pausing the engine (so
       * on a user action) */
#if 0
      /* add timer to keep rescanning */
      self->rescan_timeout_id = g_timeout_add_seconds (
        7, (GSourceFunc) hardware_processor_rescan_ext_ports, self);
#endif
    }
  else if (!activate && self->rescan_timeout_id)
    {
      /* remove timeout */
      g_source_remove (self->rescan_timeout_id);
      self->rescan_timeout_id = 0;
    }

  self->activated = activate;
}

/**
 * Processes the data.
 */
void
hardware_processor_process (HardwareProcessor * self, nframes_t nframes)
{
  /* go through each selected port and fetch data */
  for (int i = 0; i < self->num_audio_ports; i++)
    {
      ExtPort * ext_port = self->ext_audio_ports[i];
      if (!ext_port->active)
        continue;

      Port * port = self->audio_ports[i];

      /* clear the buffer */
      port_clear_buffer (port);

      switch (AUDIO_ENGINE->audio_backend)
        {
#ifdef HAVE_JACK
        case AUDIO_BACKEND_JACK:
          port_receive_audio_data_from_jack (port, 0, nframes);
          break;
#endif
#ifdef HAVE_RTAUDIO
        case AUDIO_BACKEND_ALSA_RTAUDIO:
        case AUDIO_BACKEND_JACK_RTAUDIO:
        case AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AUDIO_BACKEND_ASIO_RTAUDIO:
          /* extract audio data from the RtAudio
           * device ring buffer into RtAudio
           * device temp buffer */
          port_prepare_rtaudio_data (port);

          /* copy data from RtAudio temp buffer
           * to normal buffer */
          port_sum_data_from_rtaudio (port, 0, nframes);
          break;
#endif
        default:
          break;
        }
    }
  for (int i = 0; i < self->num_midi_ports; i++)
    {
      ExtPort * ext_port = self->ext_midi_ports[i];
      if (!ext_port->active)
        continue;

      Port * port = self->midi_ports[i];

      /* clear the buffer */
      midi_events_clear (port->midi_events, F_NOT_QUEUED);

      switch (AUDIO_ENGINE->midi_backend)
        {
#ifdef HAVE_JACK
        case MIDI_BACKEND_JACK:
          port_receive_midi_events_from_jack (port, 0, nframes);
          break;
#endif
#ifdef HAVE_RTMIDI
        case MIDI_BACKEND_ALSA_RTMIDI:
        case MIDI_BACKEND_JACK_RTMIDI:
        case MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MIDI_BACKEND_COREMIDI_RTMIDI:
#  ifdef HAVE_RTMIDI_6
        case MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          /* extract MIDI events from the RtMidi
           * device ring buffer into RtMidi device */
          port_prepare_rtmidi_events (port);

          /* copy data from RtMidi device events
           * to normal events */
          port_sum_data_from_rtmidi (port, 0, nframes);
          break;
#endif
        default:
          break;
        }
    }
}

/**
 * To be used during serialization.
 */
HardwareProcessor *
hardware_processor_clone (const HardwareProcessor * src)
{
  HardwareProcessor * self = object_new (HardwareProcessor);
  self->schema_version = HW_PROCESSOR_SCHEMA_VERSION;

  self->is_input = src->is_input;

  self->ext_audio_ports =
    object_new_n ((size_t) src->num_ext_audio_ports, ExtPort *);
  for (int i = 0; i < src->num_ext_audio_ports; i++)
    {
      self->ext_audio_ports[i] = ext_port_clone (src->ext_audio_ports[i]);
    }
  self->num_ext_audio_ports = src->num_ext_audio_ports;

  self->ext_midi_ports =
    object_new_n ((size_t) src->num_ext_midi_ports, ExtPort *);
  for (int i = 0; i < src->num_ext_midi_ports; i++)
    {
      self->ext_midi_ports[i] = ext_port_clone (src->ext_midi_ports[i]);
    }
  self->num_ext_midi_ports = src->num_ext_midi_ports;

  self->audio_ports = object_new_n ((size_t) src->num_audio_ports, Port *);
  for (int i = 0; i < src->num_audio_ports; i++)
    {
      self->audio_ports[i] = port_clone (src->audio_ports[i]);
    }
  self->num_audio_ports = src->num_audio_ports;

  self->midi_ports = object_new_n ((size_t) src->num_midi_ports, Port *);
  for (int i = 0; i < src->num_midi_ports; i++)
    {
      self->midi_ports[i] = port_clone (src->midi_ports[i]);
    }
  self->num_midi_ports = src->num_midi_ports;

  return self;
}

void
hardware_processor_free (HardwareProcessor * self)
{
  for (int i = 0; i < self->num_selected_midi_ports; i++)
    {
      g_free_and_null (self->selected_midi_ports[i]);
    }
  object_zero_and_free_if_nonnull (self->selected_midi_ports);

  for (int i = 0; i < self->num_selected_audio_ports; i++)
    {
      g_free_and_null (self->selected_audio_ports[i]);
    }
  object_zero_and_free_if_nonnull (self->selected_audio_ports);

  for (int i = 0; i < self->num_ext_audio_ports; i++)
    {
      object_free_w_func_and_null (ext_port_free, self->ext_audio_ports[i]);
    }
  object_zero_and_free_if_nonnull (self->ext_audio_ports);

  for (int i = 0; i < self->num_ext_midi_ports; i++)
    {
      object_free_w_func_and_null (ext_port_free, self->ext_midi_ports[i]);
    }
  object_zero_and_free_if_nonnull (self->ext_midi_ports);

  for (int i = 0; i < self->num_audio_ports; i++)
    {
      object_free_w_func_and_null (port_free, self->audio_ports[i]);
    }
  object_zero_and_free_if_nonnull (self->audio_ports);

  for (int i = 0; i < self->num_midi_ports; i++)
    {
      object_free_w_func_and_null (port_free, self->midi_ports[i]);
    }
  object_zero_and_free_if_nonnull (self->midi_ports);

  object_zero_and_free (self);
}
