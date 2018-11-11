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

#define M_PIF 3.14159265358979323846f

#include <stdlib.h>
#include <string.h>
#if _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif
#include <math.h>

#include "project.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/pan.h"
#include "audio/port.h"
#include "plugins/plugin.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

#include <jack/jack.h>

#define SLEEPTIME 1

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

/**
 * Creates port (used when loading projects).
 */
Port *
port_get_or_create_blank (int id)
{
  if (PROJECT->ports[id])
    {
      return PROJECT->ports[id];
    }
  else
    {
      Port * port = calloc (1, sizeof (Port));

      port->id = id;
      PROJECT->ports[id] = port;
      PROJECT->num_ports++;
      port->buf = calloc (AUDIO_ENGINE->block_length, sizeof (sample_t));
      port->nframes = AUDIO_ENGINE->block_length;
      port->num_dests = 0;
      port->flow = FLOW_UNKNOWN;

      g_message ("[port_new] Creating blank port %d", id);

      return port;
    }
}

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
Port *
port_new (nframes_t nframes, char * label)
{
  Port * port = calloc (1, sizeof (Port));

  port->id = PROJECT->num_ports;
  PROJECT->ports[PROJECT->num_ports++] = port;
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
port_free (Port * port)
{
  port_disconnect_all (port);

  if (port->label)
    g_free (port->label);
  if (port->buf)
    free (port->buf);

  array_delete ((void **) PROJECT->ports, &PROJECT->num_ports, port);
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
  port_disconnect (src, dest);
  if (src->type != dest->type)
    {
      g_error ("Cannot connect ports, incompatible types");
      return -1;
    }
  src->dests[src->num_dests++] = dest;
  dest->srcs[dest->num_srcs++] = src;
  g_message ("Connected port %d(%s) to %d(%s)",
             src->id,
             src->label,
             dest->id,
             dest->label);
  return 0;
}

/*static void*/
/*array_delete (Port ** array, int * size, Port * element)*/
/*{*/
  /*for (int i = 0; i < (* size); i++)*/
    /*{*/
      /*if (array[i] == element)*/
        /*{*/
          /*--(* size);*/
          /*for (int j = i; j < (* size); j++)*/
            /*{*/
              /*array[j] = array[j + 1];*/
            /*}*/
          /*break;*/
        /*}*/
    /*}*/
/*}*/

/**
 * Disconnects src from dest.
 */
int
port_disconnect (Port * src, Port * dest)
{
  g_assert (src);
  g_assert (dest);
  /* disconnect dest from src */
  array_delete ((void **)src->dests, &src->num_dests, dest);
  array_delete ((void **)dest->srcs, &dest->num_srcs, src);
  return 0;
}

int
ports_connected (Port * src, Port * dest)
{
  if (array_contains ((void **)src->dests,
                      src->num_dests,
                      dest))
    return 1;
  return 0;
}

/**
 * Disconnects all srcs and dests from port.
 */
int
port_disconnect_all (Port * port)
{
  g_assert (port);

  FOREACH_SRCS (port)
    {
      Port * src = port->srcs[i];
      port_disconnect (src, port);
    }

  FOREACH_DESTS (port)
    {
      Port * dest = port->dests[i];
      port_disconnect (port, dest);
    }

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
 * Apply given fader value to port.
 */
void
port_apply_fader (Port * port, float amp)
{
  for (int i = 0; i < port->nframes; i++)
    {
      port->buf[i] *= amp;
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
      if (src_port->is_piano_roll)
        {
        }
      else if (src_port->owner_pl)
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
  for (int i = 0; i < PROJECT->num_ports; i++)
    {
      Port * src = PROJECT->ports[i];
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
  if (port->type == TYPE_AUDIO)
    {
      /* FIXME sometimes this fails */
      if (port->buf)
        memset (port->buf, 0, port->nframes * sizeof (sample_t));
    }
  else if (port->type == TYPE_EVENT)
    {
      port->midi_events.num_events = 0;
    }
}

/**
 * Applies the pan to the given L/R ports.
 */
void
port_apply_pan_stereo (Port *       l,
                       Port *       r,
                       float        pan,
                       PanLaw       pan_law,
                       PanAlgorithm pan_algo)
{
  if (pan_algo == PAN_ALGORITHM_SINE_LAW)
    {
      for (int i = 0; i < l->nframes; i++)
        {
          r->buf[i] *= sinf (pan * (M_PIF / 2.f));
          l->buf[i] *= sinf ((1.f - pan) * (M_PIF / 2.f));
        }

    }
}

