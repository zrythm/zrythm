/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 * Implementation of Port API.
 * Ports are passed when processing audio signals. They carry buffers
 * with data */

#define M_PIF 3.14159265358979323846f

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "project.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/pan.h"
#include "audio/port.h"
#include "plugins/plugin.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

#define SLEEPTIME_USEC 60

/**
 * Inits ports just loaded from yml.
 */
void
port_init_loaded (Port * port)
{
  /* set caches */
  for (int j = 0; j < port->num_srcs; j++)
    port->srcs[j] =
      project_get_port (port->src_ids[j]);
  for (int j = 0; j < port->num_dests; j++)
    port->dests[j] =
      project_get_port (port->dest_ids[j]);
  port->owner_pl =
    project_get_plugin (port->owner_pl_id);
  port->owner_ch =
    project_get_channel (port->owner_ch_id);
}

void
stereo_ports_init_loaded (StereoPorts * sp)
{
  sp->l =
    project_get_port (sp->l_id);
  port_init_loaded (sp->l);
  sp->r =
    project_get_port (sp->r_id);
  port_init_loaded (sp->l);
}

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
Port *
port_new (char * label)
{
  Port * port = calloc (1, sizeof (Port));

  port->owner_pl_id = -1;
  port->owner_ch_id = -1;

  port->num_dests = 0;
  port->buf =
    calloc (AUDIO_ENGINE->block_length,
            sizeof (float));
  port->flow = FLOW_UNKNOWN;
  port->label = g_strdup (label);

  g_message ("[port_new] Creating port %s", port->label);
  project_add_port (port);

  return port;
}

/**
 * Creates port.
 */
Port *
port_new_with_type (PortType     type,
                    PortFlow     flow,
                    char         * label)
{
  Port * port = port_new (label);

  port->type = type;
  if (port->type == TYPE_EVENT)
    port->midi_events = midi_events_new (1);
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
  sp->l_id = l->id;
  sp->r = r;
  sp->r_id = r->id;

  return sp;
}

/**
 * Creates port and adds given data to it.
 */
Port *
port_new_with_data (PortInternalType internal_type, ///< the internal data format
                    PortType     type,
                    PortFlow     flow,
                    char         * label,
                    void         * data)   ///< the data
{
  Port * port = port_new_with_type (type, flow, label);

  /** TODO semaphore **/
  port->data = data;
  port->internal_type = internal_type;

  /** TODO end semaphore */

  return port;
}

/**
 * Sets the owner channel & its ID.
 */
void
port_set_owner_channel (Port *    port,
                        Channel * chan)
{
  port->owner_ch = chan;
  port->owner_ch_id = chan->id;
}

/**
 * Sets the owner plugin & its ID.
 */
void
port_set_owner_plugin (Port *   port,
                       Plugin * pl)
{
  port->owner_pl = pl;
  port->owner_pl_id = pl->id;
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
  g_warn_if_fail (src != NULL);
  g_warn_if_fail (dest != NULL);
  port_disconnect (src, dest);
  if (src->type != dest->type)
    {
      g_warning ("Cannot connect ports, incompatible types");
      return -1;
    }
  src->dests[src->num_dests] = dest;
  src->dest_ids[src->num_dests++] = dest->id;
  dest->srcs[dest->num_srcs] = src;
  dest->src_ids[dest->num_srcs++] = src->id;
  g_message ("Connected port %d(%s) to %d(%s)",
             src->id,
             src->label,
             dest->id,
             dest->label);
  return 0;
}

/**
 * Disconnects src from dest.
 */
int
port_disconnect (Port * src, Port * dest)
{
  if (!src || !dest)
    g_warning ("port_disconnect: src or dest is NULL");

  /* disconnect dest from src */
  int size;
  array_delete (src->dests, src->num_dests, dest);
  size = src->num_dests;
  array_delete (src->dest_ids, size, dest->id);
  size = src->num_srcs;
  array_delete (dest->srcs, dest->num_srcs, src);
  array_delete (src->src_ids, size, src->id);
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
  if (!port)
    g_warning ("port_disconnect_all: port is NULL");

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
 * Apply given fader value to port.
 */
void inline
port_apply_fader (Port * port, float amp)
{
  for (int i = 0; i < AUDIO_ENGINE->block_length;
       i++)
    {
      if (port->buf[i] != 0.f)
        port->buf[i] *= amp;
    }
}


/**
 * First sets port buf to 0, then sums the given port signal from its inputs.
 */
void
port_sum_signal_from_inputs (Port * port)
{
  Port * src_port;

  /* for any output port pointing to it */
  for (int k = 0; k < port->num_srcs; k++)
    {
      src_port = port->srcs[k];

      /* sum the signals */
      if (port->type == TYPE_AUDIO)
        {
          for (int l = 0;
               l < AUDIO_ENGINE->block_length; l++)
            {
              port->buf[l] += src_port->buf[l];
            }
        }
      else if (port->type == TYPE_EVENT)
        {
          /*if (src_port == AUDIO_ENGINE->*/
              /*midi_editor_manual_press &&*/
              /*AUDIO_ENGINE->midi_editor_manual_press->midi_events->num_events > 0)*/
            /*g_message ("attempting to add %d events, engine cycle %ld",*/
                       /*src_port->midi_events->num_events,*/
                       /*AUDIO_ENGINE->cycle);*/
          midi_events_append (src_port->midi_events,
                              port->midi_events);
          /*if (port->midi_events->num_events > 0)*/
          /*g_message ("appended %d events from %s to %s, cycle %ld",*/
                     /*port->midi_events->num_events,*/
                     /*src_port->label,*/
                     /*port->label,*/
                     /*AUDIO_ENGINE->cycle);*/
        }
    }
}

/**
 * Prints all connections.
 */
void
port_print_connections_all ()
{
  Port * src, * dest;
  int i, j;

  for (i = 0; i < PROJECT->num_ports; i++)
    {
      src = project_get_port (i);
      if (!src->owner_pl &&
          !src->owner_ch &&
          !src->owner_backend)
        {
          g_warning ("Port %s has no owner",
                     src->label);
        }
      for (j = 0; j < src->num_dests; j++)
        {
          dest = src->dests[j];
          g_warn_if_fail (dest);
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
  if (port->type == TYPE_AUDIO && port->buf)
    {
      memset (
        port->buf, 0,
        AUDIO_ENGINE->block_length *
          sizeof (float));
      return;
    }
  if (port->type == TYPE_EVENT)
    {
      if (port->midi_events)
        {
          port->midi_events->num_events = 0;
          g_atomic_int_set (
            &port->midi_events->processed, 0);
        }
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
  int nframes = AUDIO_ENGINE->block_length;
  if (pan_algo == PAN_ALGORITHM_SINE_LAW)
    {
      for (int i = 0; i < nframes; i++)
        {
          if (r->buf[i] != 0.f)
            r->buf[i] *= sinf (pan * (M_PIF / 2.f));
          if (l->buf[i] != 0.f)
            l->buf[i] *= sinf ((1.f - pan) * (M_PIF / 2.f));
        }

    }
}

/**
 * Applies the pan to the given L/R ports.
 */
inline void
port_apply_pan (
  Port *       port,
  float        pan,
  PanLaw       pan_law,
  PanAlgorithm pan_algo)
{
  if (pan_algo == PAN_ALGORITHM_SINE_LAW)
    {
      for (int i = 0; i < AUDIO_ENGINE->block_length;
           i++)
        {
          if (port->buf[i] == 0.f)
            continue;

          if (port->flags & PORT_FLAG_STEREO_R)
            {
              port->buf[i] *=
                sinf (pan * (M_PIF / 2.f));
              continue;
            }

          port->buf[i] *=
            sinf ((1.f - pan) * (M_PIF / 2.f));
        }
    }
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

  free (port);

  for (int i = 0; i < MIXER->master->stereo_in->l->num_srcs; i++)
    if (MIXER->master->stereo_in->l->srcs[i] == NULL)
      g_warning ("is null");
}
