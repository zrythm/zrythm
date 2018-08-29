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

#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "plugins/plugin.h"

#include <jack/jack.h>

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
Port *
port_new (nframes_t nframes)
{
  Port * port = calloc (1, sizeof (Port));

  AUDIO_ENGINE->ports[AUDIO_ENGINE->num_ports++] = port;
  /*port->id = MIXER->num_ports++;*/
  port->num_dests = 0;
  port->nframes = nframes;
  port->buf = calloc (nframes, sizeof (sample_t));
  port->flow = FLOW_UNKNOWN;

  return port;
}

/**
 * Creates port.
 */
Port *
port_new_with_type (nframes_t    nframes,
                    PortType     type,
                    PortFlow     flow)
{
  Port * port = port_new (nframes);

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
                    PortInternal internal, ///< the internal data format
                    PortType     type,
                    PortFlow     flow,
                    void         * data)   ///< the data
{
  Port * port = port_new_with_type (nframes, type, flow);
  port->data = data;
  port->internal = internal;

  return port;
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_delete (Port * port)
{
  /* TODO delete from mixer */
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
  src->dests[src->num_dests++] = dest;
  dest->srcs[dest->num_srcs++] = src;
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
  /* disconnect dest from src */
  array_delete (src->dests, &src->num_dests, dest);
  array_delete (dest->srcs, &dest->num_srcs, src);
}

/**
 * if port buffer size changed, reallocate port buffer, otherwise memset to 0.
 */
void
port_init_buf (Port *port, nframes_t nframes)
{
    /* if port buf size changed, reallocate */
    if (port->nframes != nframes ||
        port->nframes == 0)
      {
        if (port->nframes > 0)
          free (port->buf);
        port->buf = calloc (nframes, sizeof (sample_t));
        port->nframes = nframes;
      }
    else /* otherwise memset to 0 */
      {
        memset (port->buf, '\0', nframes);
      }
}


/**
 * First sets port buf to 0, then sums the given port signal from its inputs.
 */
void
port_sum_signal_from_inputs (Port * port, nframes_t nframes)
{
  port_init_buf (port, nframes);

  /* for any output port pointing to it */
  for (int k = 0; k < port->num_srcs; k++)
    {
      Port * src_port = port->srcs[k];

      /* wait for owner to finish processing */
      if (src_port->owner_pl)
        while (!src_port->owner_pl->processed)
          {
            sleep (5);
          }
      else if (src_port->owner_ch)
        while (!src_port->owner_ch->processed)
          {
            sleep (5);
          }

      /* sum the signals */
      for (int l = 0; l < nframes; l++)
        {
          port->buf[l] += src_port->buf[l];
        }
    }
}
