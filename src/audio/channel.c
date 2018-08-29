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

#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#include "audio/channel.h"
#include "audio/mixer.h"
#include "gui/widgets/channel.h"

#include <gtk/gtk.h>

/**
 * Calculate decibels using RMS method.
 */
static double
calculate_rms_db (sample_t * buf, nframes_t nframes)
{
  double sum = 0;
  for (int i = 0; i < nframes; i += 2)
  {
      double sample = buf[i] / 32768.0;
      sum += (sample * sample);
  }
  double rms = sqrt (sum / (nframes / 2));
  return 20 * log10(rms);
}

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
              nframes_t   nframes)    ///< sample count
{
  /* sum AUDIO IN coming to the channel */
  port_sum_signal_from_inputs (channel->stereo_in->l, nframes);
  port_sum_signal_from_inputs (channel->stereo_in->r, nframes);

  /* go through each slot (plugin) on the channel strip */
  for (int i = 0; i < channel->num_plugins; i++)
    {
      Plugin * plugin = channel->strip[i];
      if (plugin)
        {

          /* sum incoming signals for each input port */
          for (int j = 0; j < plugin->num_in_ports; j++)
            {
              Port * port = plugin->in_ports[j];

              port_sum_signal_from_inputs (port, nframes);
            }

            /* run plugin processing
             * this should put the appropriate result in the plugin's audio out */
            plugin_process (plugin, nframes);
        }
    }

  /* same for channel ports */
  port_sum_signal_from_inputs (channel->stereo_out->l, nframes);
  port_sum_signal_from_inputs (channel->stereo_out->r, nframes);

  /* calc decibels */
  channel->l_port_db = calculate_rms_db (channel->stereo_out->l->buf,
                                         channel->stereo_out->l->nframes);
  channel->r_port_db = calculate_rms_db (channel->stereo_out->r->buf,
                                         channel->stereo_out->r->nframes);
}

static Channel *
_create_channel ()
{
  Channel * channel = calloc (1, sizeof (Channel));

  /* create ports */
  channel->stereo_in = stereo_ports_new (
              port_new_with_type (AUDIO_ENGINE->block_length,
                                  TYPE_AUDIO, FLOW_INPUT),
              port_new_with_type (AUDIO_ENGINE->block_length,
                                  TYPE_AUDIO, FLOW_INPUT));
  channel->midi_in = port_new_with_type (AUDIO_ENGINE->block_length,
                                         TYPE_EVENT, FLOW_INPUT);
  channel->stereo_out = stereo_ports_new (
              port_new_with_type (AUDIO_ENGINE->block_length,
                                  TYPE_AUDIO, FLOW_OUTPUT),
              port_new_with_type (AUDIO_ENGINE->block_length,
                                  TYPE_AUDIO, FLOW_OUTPUT));

/* init plugins */
for (int i = 0; i < MAX_PLUGINS; i++)
  {
    channel->strip[i] = NULL;
  }

  channel->volume = 0.0f;
  channel->phase = 0.0f;

  /* connect MIDI in port from engine's jack port */
  g_message ("connecting engine MIDI IN port to %s MIDI IN",
             channel->name);
  port_connect (AUDIO_ENGINE->midi_in, channel->midi_in);

  return channel;
}

/**
 * Creates master channel
 */
Channel *
channel_create_master ()
{
  g_message ("Creating Master channel");

  Channel * channel = _create_channel();

  channel->type = CT_MASTER;

  /* default settings */
  gdk_rgba_parse (&channel->color, "red");
  channel->name = "Master";
  channel->output = NULL;

  /* connect stereo out ports to engine's jack ports */
  /* TODO connect to monitor, and then from monitor to engine */
  /*g_message ("Connecting %s stereo outs to engine stereo outs",*/

  /*port_connect (channel->stereo_out->l, AUDIO_ENGINE->stereo_out->l);*/
  /*port_connect (channel->stereo_out->r, AUDIO_ENGINE->stereo_out->r);*/

  return channel;
}

/**
 * Creates a channel using the given params
 */
Channel *
channel_create (int     type)             ///< the channel type (AUDIO/INS)
{
  g_message ("Creating channel of type %i", type);

  Channel * channel = _create_channel();

  channel->type = type;

  gdk_rgba_parse (&channel->color, "rgb(20%,60%,4%)");
  channel-> name = g_strdup_printf ("Ch %d", ++MIXER->num_channels);
  channel->output = MIXER->master;

  /* connect channel out ports to master */
  g_message ("Connecting %s stereo out ports to Master",
             channel->name);
  port_connect (channel->stereo_out->l,
                MIXER->master->stereo_in->l);
  port_connect (channel->stereo_out->r,
                MIXER->master->stereo_in->r);

  return channel;
}

void
channel_set_phase (void * _channel, float phase)
{
  Channel * channel = (Channel *) _channel;
  channel->phase = phase;
  gtk_label_set_text (channel->widget->phase_reading,
                      g_strdup_printf ("%.1f", phase));
}

float
channel_get_phase (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->phase;
}

void
channel_set_volume (void * _channel, float volume)
{
  Channel * channel = (Channel *) _channel;
  channel->volume = volume;
  /* TODO update tooltip */
  gtk_label_set_text (channel->widget->phase_reading,
                      g_strdup_printf ("%.1f", volume));
}

float
channel_get_volume (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->volume;
}

float
channel_get_current_l_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->l_port_db;
}

float
channel_get_current_r_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->r_port_db;
}

/**
 * Adds given plugin to given position in the strip.
 *
 * The plugin must be already instantiated at this point.
 */
void
channel_add_plugin (Channel * channel,    ///< the channel
                    int         pos,     ///< the position in the strip
                                        ///< (starting from 0)
                    Plugin      * plugin  ///< the plugin to add
                    )
{
  /* free current plugin */
  if (channel->strip[pos])
    {
      g_message ("Removing %s from %s:%d", channel->strip[pos]->descr->name,
                 channel->name, pos);
      plugin_free (channel->strip[pos]);
    }

  g_message ("Inserting %s at %s:%d", plugin->descr->name,
             channel->name, pos);
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
              g_message ("Connecting %s output port %d to %s input port %d",
                         prev_plugin->descr->name, i,
                         plugin->descr->name, i);
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

  Plugin * next_plugin;
  for (int i = 1; i < MAX_PLUGINS; i++)
    {
      next_plugin = channel->strip[pos + i];
      if (next_plugin)
        break;
    }

  /* automatically connect the output ports of the plugin
   * to the input ports of the next plugin sequentially */

  /* for each output port of the plugin */
  if (next_plugin)
    {
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

  /* if last plugin, connect to AUDIO_OUT */

  channel->num_plugins++;
}


