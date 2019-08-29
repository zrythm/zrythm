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

#include "config.h"

#include "audio/engine.h"
#include "audio/ext_port.h"
#include "project.h"

#ifdef HAVE_JACK
#include <jack/jack.h>
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
  int       nframes)
{
  switch (self->type)
    {
    case EXT_PORT_TYPE_JACK:
      return
        (float *)
        jack_port_get_buffer (
          self->jport,
          nframes);
      break;
    case EXT_PORT_TYPE_ALSA:
    default:
      g_return_val_if_reached (NULL);
      break;
    }
}

/**
 * Clears the buffer of the external port.
 */
void
ext_port_clear_buffer (
  ExtPort * ext_port,
  int       nframes)
{
  float * buf =
    ext_port_get_buffer (ext_port, nframes);
  g_message ("clearing buffer of %p", ext_port);

  memset (
    buf, 0,
    nframes * sizeof (float));
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

/**
 * Collects external ports of the given type.
 *
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
  switch (AUDIO_ENGINE->audio_backend)
    {
    case AUDIO_BACKEND_JACK:
      {
#ifdef HAVE_JACK
        int flags = 0;
        if (hw)
          flags |= JackPortIsPhysical;
        if (flow == FLOW_INPUT)
          flags |= JackPortIsInput;
        else if (flow == FLOW_OUTPUT)
          flags |= JackPortIsOutput;
        char * jtype = NULL;
        if (type == TYPE_AUDIO)
          jtype = JACK_DEFAULT_AUDIO_TYPE;
        else if (type == TYPE_EVENT)
          jtype = JACK_DEFAULT_MIDI_TYPE;

        const char ** ports =
          jack_get_ports (
            AUDIO_ENGINE->client,
            NULL, jtype, flags);

        int i = 0;
        char * pname;
        jack_port_t * jport;
        while ((pname = (char *) ports[i]) != NULL)
          {
            jport =
              jack_port_by_name (
                AUDIO_ENGINE->client,
                pname);

            arr[i] =
              ext_port_from_jack_port (jport);

            i++;
          }

        *size = i;
#endif
      }
      break;
    case AUDIO_BACKEND_ALSA:
      break;
    default:
      break;
    }
}

#ifdef HAVE_JACK

/**
 * Creates an ExtPort from a JACK port.
 */
ExtPort *
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
    (char*) malloc (jack_port_name_size ());
  aliases[1] =
    (char*) malloc (jack_port_name_size ());
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

  return self;
}

#endif

/**
 * Frees an array of ExtPort pointers.
 */
void
ext_ports_free (
  ExtPort ** ext_port,
  int        size)
{
  /* TODO */
}

/**
 * Frees the ext_port.
 */
void
ext_port_free (
  ExtPort * ext_port)
{
  /* TODO */
}
