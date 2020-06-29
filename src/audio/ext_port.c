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

#include "zrythm-config.h"

#include "audio/engine.h"
#include "audio/engine_jack.h"
#include "audio/engine_rtaudio.h"
#include "audio/engine_rtmidi.h"
#include "audio/ext_port.h"
#include "audio/rtaudio_device.h"
#include "audio/rtmidi_device.h"
#include "audio/windows_mme_device.h"
#include "project.h"
#include "zrythm_app.h"

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

/**
 * Inits the ExtPort after loading a project.
 */
void
ext_port_init_loaded (
  ExtPort * ext_port)
{
  /* TODO */
}

/**
 * Returns the buffer of the external port.
 */
float *
ext_port_get_buffer (
  ExtPort * self,
  nframes_t nframes)
{
  switch (self->type)
    {
#ifdef HAVE_JACK
    case EXT_PORT_TYPE_JACK:
      return
        (float *)
        jack_port_get_buffer (
          self->jport,
          nframes);
      break;
#endif
#ifdef HAVE_ALSA
    case EXT_PORT_TYPE_ALSA:
#endif
#ifdef HAVE_RTMIDI
    case EXT_PORT_TYPE_RTMIDI:
#endif
    default:
      g_return_val_if_reached (NULL);
      break;
    }
g_return_val_if_reached (NULL);
}

/**
 * Clears the buffer of the external port.
 */
void
ext_port_clear_buffer (
  ExtPort * ext_port,
  nframes_t nframes)
{
  float * buf =
    ext_port_get_buffer (ext_port, nframes);
  if (!buf)
    return;

  g_message ("clearing buffer of %p", ext_port);

  memset (buf, 0, nframes * sizeof (float));
}

/**
 * Exposes the given Port if not exposed and makes
 * the connection from the Port to the ExtPort (eg in
 * JACK) or backwards.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_connect (
  ExtPort * ext_port,
  Port *    port,
  int       src)
{
  /* TODO */
}

/**
 * Disconnects the Port from the ExtPort.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_disconnect (
  ExtPort * ext_port,
  Port *    port,
  int       src)
{
  /* TODO */
}

#ifdef HAVE_JACK
/**
 * Creates an ExtPort from a JACK port.
 */
static ExtPort *
ext_port_from_jack_port (
  jack_port_t * jport)
{
  ExtPort * self =
    calloc (1, sizeof (ExtPort));

  self->jport = jport;
  self->full_name =
    g_strdup (jack_port_name (jport));
  self->short_name  =
    g_strdup (jack_port_short_name (jport));
  self->type = EXT_PORT_TYPE_JACK;

  char * aliases[2];
  aliases[0] =
    (char*)
    malloc (
      (size_t) jack_port_name_size ());
  aliases[1] =
    (char*)
    malloc (
      (size_t) jack_port_name_size ());
  self->num_aliases =
    jack_port_get_aliases (
      jport, aliases);

  if (self->num_aliases == 2)
    {
      self->alias2 = g_strdup (aliases[1]);
      self->alias1 = g_strdup (aliases[0]);
    }
  else if (self->num_aliases == 1)
    self->alias1 = g_strdup (aliases[0]);

  free (aliases[0]);
  free (aliases[1]);

  return self;
}

static void
get_ext_ports_from_jack (
  PortType   type,
  PortFlow   flow,
  int        hw,
  ExtPort ** arr,
  int *      size)
{
  long unsigned int flags = 0;
  if (hw)
    flags |= JackPortIsPhysical;
  if (flow == FLOW_INPUT)
    flags |= JackPortIsInput;
  else if (flow == FLOW_OUTPUT)
    flags |= JackPortIsOutput;
  const char * jtype =
    engine_jack_get_jack_type (type);
  if (!jtype)
    return;

  const char ** ports =
    jack_get_ports (
      AUDIO_ENGINE->client,
      NULL, jtype, flags);

  if (!ports)
    return;

  int i = 0;
  jack_port_t * jport;
  while (ports[i] != NULL)
    {
      jport =
        jack_port_by_name (
          AUDIO_ENGINE->client,
          ports[i]);

      arr[*size + i] =
        ext_port_from_jack_port (jport);

      i++;
    }

  jack_free (ports);

  *size += i;
}
#endif

#ifdef _WOE32
/**
 * Creates an ExtPort from a JACK port.
 */
static ExtPort *
ext_port_from_windows_mme_device (
  WindowsMmeDevice * dev)
{
  ExtPort * self =
    calloc (1, sizeof (ExtPort));

  self->mme_dev = dev;
  self->full_name = g_strdup (dev->name);
  self->type = EXT_PORT_TYPE_WINDOWS_MME;

  return self;
}

static void
get_ext_ports_from_windows_mme (
  PortFlow   flow,
  ExtPort ** arr,
  int *      size)
{
  WindowsMmeDevice * dev;
	    int i = 0;
  if (flow == FLOW_OUTPUT)
    {
      for (i = 0;
           i < AUDIO_ENGINE->num_mme_in_devs; i++)
        {
          dev = AUDIO_ENGINE->mme_in_devs[i];
          g_return_if_fail (dev);
          arr[*size + i] =
            ext_port_from_windows_mme_device (dev);
        }
      (*size) += i;
    }
  else if (flow == FLOW_INPUT)
    {
      for (i = 0;
           i < AUDIO_ENGINE->num_mme_out_devs; i++)
        {
          dev = AUDIO_ENGINE->mme_out_devs[i];
          g_return_if_fail (dev);
          arr[*size + i] =
            ext_port_from_windows_mme_device (dev);
        }
      (*size) += i;
    }
}
#endif

#ifdef HAVE_RTMIDI
/**
 * Creates an ExtPort from a RtMidi port.
 */
static ExtPort *
ext_port_from_rtmidi (
  unsigned int id)
{
  ExtPort * self =
    calloc (1, sizeof (ExtPort));

  RtMidiDevice * dev =
    rtmidi_device_new (1, id, NULL);
  self->rtmidi_id = id;
  self->full_name =
    g_strdup (
      rtmidi_get_port_name (dev->in_handle, id));
  self->type = EXT_PORT_TYPE_RTMIDI;
  rtmidi_device_free (dev);

  return self;
}

static void
get_ext_ports_from_rtmidi (
  PortFlow   flow,
  ExtPort ** arr,
  int *      size)
{
  if (flow == FLOW_OUTPUT)
    {
      unsigned int num_ports =
        engine_rtmidi_get_num_in_ports (
          AUDIO_ENGINE);
      unsigned int i;
      for (i = 0; i < num_ports; i++)
        {
          arr[*size + (int) i] =
            ext_port_from_rtmidi (i);
        }
      (*size) += (int) i;
    }
  else if (flow == FLOW_INPUT)
    {
      *size = 0;
    }
}
#endif

#ifdef HAVE_RTAUDIO
/**
 * Creates an ExtPort from a RtAudio port.
 */
static ExtPort *
ext_port_from_rtaudio (
  unsigned int id,
  unsigned int channel_idx,
  int          is_input,
  int          is_duplex)
{
  ExtPort * self =
    calloc (1, sizeof (ExtPort));

  RtAudioDevice * dev =
    rtaudio_device_new (1, id, channel_idx, NULL);
  self->rtaudio_id = id;
  self->rtaudio_channel_idx = channel_idx;
  self->rtaudio_is_input = is_input;
  self->rtaudio_is_duplex = is_duplex;
  self->full_name =
    g_strdup_printf (
      "%s (in %d)", dev->name, channel_idx);
  self->type = EXT_PORT_TYPE_RTAUDIO;
  rtaudio_device_free (dev);

  return self;
}

static void
get_ext_ports_from_rtaudio (
  PortFlow   flow,
  ExtPort ** arr,
  int *      size)
{
  /* note: this is an output port from the graph
   * side that will be used as an input port on
   * the zrythm side */
  if (flow == FLOW_OUTPUT)
    {
      rtaudio_t rtaudio =
        engine_rtaudio_create_rtaudio (
          AUDIO_ENGINE);
      int num_devs = rtaudio_device_count (rtaudio);
      for (unsigned int i = 0;
           i < (unsigned int) num_devs; i++)
        {
          rtaudio_device_info_t dev_nfo =
            rtaudio_get_device_info (
              rtaudio, (int) i);
          if (dev_nfo.input_channels > 0)
            {
              for (unsigned int j = 0;
                   j < dev_nfo.input_channels; j++)
                {
                  arr[*size] =
                    ext_port_from_rtaudio (
                      i, j, true, false);
                  (*size)++;
                  g_message ("\n\n found input device");
                }
#if 0
              for (unsigned int j = 0;
                   j < dev_nfo.duplex_channels; j++)
                {
                  arr[*size] =
                    ext_port_from_rtaudio (
                      i, j, false, true);
                  (*size)++;
                  g_message ("\n\n found duplex device");
                }
#endif
            }
          else
            {
              continue;
            }
        }
    }
  else if (flow == FLOW_INPUT)
    {
      *size = 0;
    }
}
#endif

/**
 * Collects external ports of the given type.
 *
 * @param flow The signal flow. Note that this is
 *   inverse to what Zrythm sees. E.g., to get
 *   MIDI inputs like MIDI keyboards, pass
 *   \ref FLOW_OUTPUT here.
 * @param hw Hardware or not.
 * @param ports An array of ExtPort pointers to fill
 *   in. The array should be preallocated.
 * @param size Size of the array to fill in.
 */
void
ext_ports_get (
  PortType   type,
  PortFlow   flow,
  int        hw,
  ExtPort ** arr,
  int *      size)
{
  *size = 0;
  if (type == TYPE_AUDIO)
    {
      switch (AUDIO_ENGINE->audio_backend)
        {
#ifdef HAVE_JACK
        case AUDIO_BACKEND_JACK:
          get_ext_ports_from_jack (
            type, flow, hw, arr, size);
          break;
#endif
#ifdef HAVE_RTAUDIO
        case AUDIO_BACKEND_RTAUDIO:
          get_ext_ports_from_rtaudio (
            flow, arr, size);
          break;
#endif
#ifdef HAVE_ALSA
        case AUDIO_BACKEND_ALSA:
          break;
#endif
        default:
          break;
        }
    }
  else if (type == TYPE_EVENT)
    {
      switch (AUDIO_ENGINE->midi_backend)
        {
#ifdef HAVE_JACK
        case MIDI_BACKEND_JACK:
          get_ext_ports_from_jack (
            type, flow, hw, arr, size);
          break;
#endif
#ifdef HAVE_ALSA
        case MIDI_BACKEND_ALSA:
          break;
#endif
#ifdef _WOE32
        case MIDI_BACKEND_WINDOWS_MME:
          get_ext_ports_from_windows_mme (
            flow, arr, size);
          break;
#endif
#ifdef HAVE_RTMIDI
        case MIDI_BACKEND_RTMIDI:
          get_ext_ports_from_rtmidi (
            flow, arr, size);
          break;
#endif
        default:
          break;
        }
    }
}

/**
 * Prints the port info.
 */
void
ext_port_print (
  ExtPort * self)
{
  g_message (
    "Ext port:\n"
    "full name: %s",
    self->full_name);
}

/**
 * Creates a shallow clone of the port.
 */
ExtPort *
ext_port_clone (
  ExtPort * ext_port)
{
  ExtPort * newport =
    calloc (1, sizeof (ExtPort));

#ifdef HAVE_JACK
  newport->jport = ext_port->jport;
#endif
#ifdef _WOE32
  newport->mme_dev = ext_port->mme_dev;
#endif
#ifdef HAVE_RTMIDI
  newport->rtmidi_id = ext_port->rtmidi_id;
#endif
#ifdef HAVE_RTAUDIO
  newport->rtaudio_id = ext_port->rtaudio_id;
  newport->rtaudio_channel_idx = ext_port->rtaudio_channel_idx;
  newport->rtaudio_is_input = ext_port->rtaudio_is_input;
  newport->rtaudio_is_duplex = ext_port->rtaudio_is_duplex;
#endif
  if (ext_port->full_name)
    newport->full_name =
      g_strdup (ext_port->full_name);
  if (ext_port->short_name)
    newport->short_name =
      g_strdup (ext_port->short_name);
  if (ext_port->alias1)
    newport->alias1 =
    g_strdup (ext_port->alias1);
  if (ext_port->alias2)
    newport->alias2 =
    g_strdup (ext_port->alias2);
  newport->num_aliases = ext_port->num_aliases;
  newport->type = ext_port->type;

  return newport;
}

/**
 * Frees an array of ExtPort pointers.
 */
void
ext_ports_free (
  ExtPort ** ext_ports,
  int        size)
{
  for (int i = 0; i < size; i++)
    {
      ext_port_free (ext_ports[i]);
    }
}

/**
 * Frees the ext_port.
 */
void
ext_port_free (
  ExtPort * self)
{
  if (self->full_name)
    g_free (self->full_name);
  if (self->short_name)
    g_free (self->short_name);
  if (self->alias1)
    g_free (self->alias1);
  if (self->alias2)
    g_free (self->alias2);

  free (self);
}
