/*
 * audio/ports.h - ports to pass when processing the audio signal
 *
 * copyright (c) 2018 alexandros theodotou
 *
 * this file is part of zrythm
 *
 * zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * zrythm is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * gnu general public license for more details.
 *
 * you should have received a copy of the gnu general public license
 * along with zrythm.  if not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 * Implementation of Port API.
 * Ports are passed when processing audio signals. They carry buffers
 * with data */

#include <stdlib.h>
#include <string.h>
#if _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif
#include <math.h>

#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "plugins/plugin.h"

#include <gtk/gtk.h>

#include <jack/jack.h>

#define SLEEPTIME 1

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
Port *
port_new (nframes_t nframes, char * label)
{
  Port * port = calloc (1, sizeof (Port));

  port->id = AUDIO_ENGINE->num_ports;
  AUDIO_ENGINE->ports[AUDIO_ENGINE->num_ports++] = port;
  port->num_dests = 0;
  port->nframes = nframes;
  port->buf = calloc (nframes, sizeof (sample_t));
  port->flow = FLOW_UNKNOWN;
  port->label = g_strdup (label);

  g_message ("[port_new] Creating port %s", port->label);

  return port;
}

/**
 * Creates port.
 */
Port *
port_new_with_type (nframes_t    nframes,
                    PortType     type,
                    PortFlow     flow,
                    char         * label)
{
  Port * port = port_new (nframes, label);

  port->type = type;
  port->flow = flow;

  return port;
}

/**
 * Creates stereo ports.
 */
StereoPorts *
stereo_ports_new (Port * l, Port * r)
{
  StereoPorts * sp = calloc (1, sizeof (StereoPorts));
  sp->l = l;
  sp->r = r;

  return sp;
}

/**
 * Creates port and adds given data to it.
 */
Port *
port_new_with_data (nframes_t    nframes,
                    PortInternalType internal_type, ///< the internal data format
                    PortType     type,
                    PortFlow     flow,
                    char         * label,
                    void         * data)   ///< the data
{
  Port * port = port_new_with_type (nframes, type, flow, label);

  /** TODO semaphore **/
  port->data = data;
  port->internal_type = internal_type;

  /** TODO end semaphore */

  return port;
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_delete (Port * port)
{
  engine_delete_port (port);

  if (port->label)
    free (port->label);
  if (port->buf)
    free (port->buf);

  free (port);
}

/**
 */
/*void*/
/*port_init (Port * port)*/
/*{*/
  /*port->id = MIXER->num_ports++;*/
  /*port->num_destinations = 0;*/
/*}*/

/**
 * Connets src to dest.
 */
int
port_connect (Port * src, Port * dest)
{
  g_message ("Connecting port %s to %s", src->label, dest->label);
  if (src->type != dest->type)
    {
      g_error ("Cannot connect ports, incompatible types");
      return -1;
    }
  src->dests[src->num_dests++] = dest;
  dest->srcs[dest->num_srcs++] = src;
  return 0;
}

static void
array_delete (Port ** array, int * size, Port * element)
{
  for (int i = 0; i < (* size); i++)
    {
      if (array[i] == element)
        {
          --(* size);
          for (int j = i; j < (* size); j++)
            {
              array[j] = array[j + 1];
            }
          break;
        }
    }
}

/**
 * Disconnects src from dest.
 */
int
port_disconnect (Port * src, Port * dest)
{
  g_assert (src);
  g_assert (dest);
  /* disconnect dest from src */
  array_delete (src->dests, &src->num_dests, dest);
  array_delete (dest->srcs, &dest->num_srcs, src);
  return 0;
}

/**
 * if port buffer size changed, reallocate port buffer, otherwise memset to 0.
 */
/*void*/
/*port_init_buf (Port *port, nframes_t nframes)*/
/*{*/
    /*[> if port buf size changed, reallocate <]*/
    /*if (port->nframes != nframes ||*/
        /*port->nframes == 0)*/
      /*{*/
        /*if (port->nframes > 0)*/
          /*free (port->buf);*/
        /*port->buf = calloc (nframes, sizeof (sample_t));*/
        /*port->nframes = nframes;*/
      /*}*/
    /*else [> otherwise memset to 0 <]*/
      /*{*/
        /*memset (port->buf, '\0', nframes);*/
      /*}*/
/*}*/

/**
 * Get amplitude from db.
 */
static float
get_amp_from_dbfs (float dbfs)
{
  float amp = (float) pow (10.0, dbfs / 20.0);
  return amp;
}

/**
 * Apply given fader value to port.
 */
void
port_apply_fader (Port * port, float dbfs)
{
  for (int i = 0; i < port->nframes; i++)
    {
      port->buf[i] *= get_amp_from_dbfs (dbfs);
    }
}


/**
 * First sets port buf to 0, then sums the given port signal from its inputs.
 */
void
port_sum_signal_from_inputs (Port * port, nframes_t nframes)
{
  /*port_init_buf (port, nframes);*/
#if _POSIX_C_SOURCE >= 199309L
  struct timespec ts;
  ts.tv_sec = SLEEPTIME / 1000;
  ts.tv_nsec = (SLEEPTIME % 1000) * 1000000;
#endif

  /* for any output port pointing to it */
  for (int k = 0; k < port->num_srcs; k++)
    {
      Port * src_port = port->srcs[k];

      /* wait for owner to finish processing */
      if (src_port->owner_pl)
        while (!src_port->owner_pl->processed)
          {
#if _POSIX_C_SOURCE >= 199309L
            nanosleep(&ts, NULL);
#else
            usleep (SLEEPTIME * 1000);
#endif
          }
      else if (src_port->owner_ch)
        {
          while (!src_port->owner_ch->processed)
            {
#if _POSIX_C_SOURCE >= 199309L
              nanosleep(&ts, NULL);
#else
              usleep (SLEEPTIME * 1000);
#endif
            }
        }

      /* sum the signals */
      if (port->type == TYPE_AUDIO)
        {
          for (int l = 0; l < nframes; l++)
            {
              port->buf[l] += src_port->buf[l];
            }
        }
      else if (port->type == TYPE_EVENT)
        {
          midi_events_append (&src_port->midi_events,
                              &port->midi_events);
        }
    }
}

/**
 * Prints all connections.
 */
void
port_print_connections_all ()
{
  for (int i = 0; i < AUDIO_ENGINE->num_ports; i++)
    {
      Port * src = AUDIO_ENGINE->ports[i];
      if (!src->owner_pl && !src->owner_ch && !src->owner_jack)
        {
          g_warning ("Port %s has no owner", src->label);
        }
      for (int j = 0; j < src->num_dests; j++)
        {
          Port * dest = src->dests[j];
          g_assert (dest);
          g_message ("%s connected to %s", src->label, dest->label);
        }
    }
}

/**
 * Clears the port buffer.
 */
void
port_clear_buffer (Port * port)
{
  memset (port->buf, 0, port->nframes * sizeof (sample_t));
}
