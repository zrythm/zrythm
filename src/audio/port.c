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

#include "audio/mixer.h"
#include "audio/port.h"

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
Port *
port_new ()
{
  Port * port = calloc (1, sizeof (Port));

  MIXER->ports[MIXER->num_ports] = port;
  port->id = MIXER->num_ports++;
  port->num_dests = 0;
  port->nframes = 0;

  return port;
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_delete (Port * port)
{
  /* TODO delete from mixer */

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

