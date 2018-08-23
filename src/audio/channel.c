/*
 * audio/channel.c - a channel on the mixer
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <unistd.h>
#include <stdlib.h>

#include "audio/channel.h"

/**
 * For each plugin
 *   -> for each input port it owns
 *     -> check if any output ports point to it (including of the previous slot)
 *       -> 1. wait for their owner to finish
 *       -> 2. sum their signals
 *     -> process plugin
 */
void
channel_process (Channel * channel,  ///< slots
              nframes_t   samples,    ///< sample count
              sample_t *  l_buf,   ///< sample buffer L
              sample_t *  r_buf)   ///< sample buffer R
{
  /* go through each slot (plugin) on the channel strip */
  for (int i = 0; i < MAX_PLUGINS; i++)
    {
      Plugin * plugin = channel->strip[i];

      /* if current slot has a plugin */
      if (!plugin_is_dummy (plugin))
        {

          /* for each input port */
          for (int j = 0; j < plugin->num_in_ports; j++)
            {
              Port * port = plugin->in_ports[j];

              /* for any output port pointing to it */
              for (int k = 0; k < port->num_srcs; k++)
                {

                  /* wait for owner plugin to finish processing */
                  while (!port->srcs[k]->owner->processed)
                    {
                      sleep (5);
                    }

                  /* sum the signals */
                  for (int l = 0; l < port->nframes; l++)
                    {
                      port->buf[l] += port->srcs[k]->buf[l];
                    }
                }
            }
        }
    }
}

/**
 * Creates a channel using the given params
 */
Channel *
channel_create (int     type)             ///< the channel type (AUDIO/INS)
{
  Channel * channel = malloc (sizeof (Channel));

  /* init plugins */
  for (int i = 0; i < MAX_PLUGINS; i++)
    {
      channel->strip[i] = plugin_new_dummy (channel);
      if (i > 0)
        {
          /* connect output ports of previous plugin to input ports
           * of this plugin */
          port_connect (channel->strip[i - 1]->out_ports[0],
                        channel->strip[i]->in_ports[0]);
          port_connect (channel->strip[i - 1]->out_ports[1],
                        channel->strip[i]->in_ports[1]);
        }
    }
  channel->type = type;
  channel->volume = 0.0;
  channel->muted = 0;
  channel->soloed = 0;
}

/**
 * Adds given plugin to given position in the strip.
 *
 * The plugin must be already initialized (but not instantiated) at this point.
 */
void
channel_add_initialized_plugin (Channel * channel,    ///< the channel
                    int         pos,     ///< the position in the strip
                                        ///< (starting from 0)
                    Plugin      * plugin  ///< the plugin to add
                    )
{
  /* free current plugin */
  plugin_free (channel->strip[pos]);

  channel->strip[pos] = plugin;

  /* ------------------------------------------------------
   * connect input ports
   * ------------------------------------------------------*/
  if (pos == 0) /* if main plugin */
    {
      if (channel->type == CT_AUDIO)
        {
          /* TODO connect L and R audio ports for recording */
        }
      else if (channel->type == CT_MIDI)
        {
          /* TODO connect MIDI port to the plugin
           * the plugin must already be initialized at this point
           * with its ports set up, so we should know where to connect
           * the MIDI signal */
        }
    }
  else
    {
      /* connect each input port from the previous plugin sequentially */
      Plugin * prev_plugin = channel->strip[pos - 1];

      /* automatically connect the output ports of the plugin
       * to the input ports of the next plugin sequentially */

      /* for each input port of the plugin */
      for (int i = 0; i < plugin->num_in_ports; i++)
        {
          /* if nth output port exists on prev plugin, connect them */
          if (prev_plugin->num_out_ports > i)
            {
              port_connect (prev_plugin->out_ports[i],
                            plugin->in_ports[i]);
            }
          /* otherwise break, no more input ports on next plugin */
          else
            {
              break;
            }
        }
    }


  /* ------------------------------------------------------
   * connect output ports
   * ------------------------------------------------------*/

  Plugin * next_plugin = channel->strip[pos + 1];

  /* automatically connect the output ports of the plugin
   * to the input ports of the next plugin sequentially */

  /* for each output port of the plugin */
  for (int i = 0; i < plugin->num_out_ports; i++)
    {
      /* if nth input port exists on next plugin, connect them */
      if (next_plugin->num_in_ports > i)
        {
          port_connect (plugin->out_ports[i],
                        next_plugin->in_ports[i]);
        }
      /* otherwise break, no more input ports on next plugin */
      else
        {
          break;
        }
    }
}


