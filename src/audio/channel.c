/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex and zrythm dot org>
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

/**
 * \file
 *
 * A channel on the mixer.
 */

#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"

#include "audio/audio_track.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/engine_jack.h"
#include "audio/instrument_track.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/pan.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

/**
 * Connects a plugin that has at least one MIDI
 * input port.
 */
static inline void
connect_midi_ins (
  Channel * ch,
  int       pos,
  Plugin *  pl)
{
  int i;
  Port * in_port;

  /* Connect MIDI port and piano roll to the
   * plugin */
  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];
      if (in_port->identifier.type == TYPE_EVENT &&
          in_port->identifier.flow == FLOW_INPUT)
        {
          port_connect (
            ch->midi_in,
            in_port);
          if (ch->type == CT_MIDI &&
              pos == 0)
            {
              port_connect (
                ch->piano_roll,
                in_port);
            }
        }
    }
}

/**
 * Disconnects a plugin that has at least one MIDI
 * input port.
 */
static inline void
disconnect_midi_ins (
  Channel * ch,
  int       pos,
  Plugin *  pl)
{
  int i;
  Port * in_port;

  /* Disconnect MIDI port and piano roll to the
   * plugin */
  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];
      if (in_port->identifier.type == TYPE_EVENT &&
          in_port->identifier.flow == FLOW_INPUT)
        {
          if (ports_connected (
                ch->midi_in, in_port))
            port_disconnect (
              ch->midi_in,
              in_port);
          if (ch->type == CT_MIDI &&
              pos == 0)
            {
              if (ports_connected (
                    ch->midi_in, in_port))
                port_disconnect (
                  ch->piano_roll,
                  in_port);
            }
        }
    }
}

/**
 * Disconnect stereo in ports from stereo out.
 *
 * Used when there is no plugin in the channel.
 */
static inline void
disconnect_stereo_in_from_stereo_out (
  Channel * ch)
{
  port_disconnect (
    ch->stereo_in->l,
    ch->stereo_out->l);
  port_disconnect (
    ch->stereo_in->r,
    ch->stereo_out->r);
}

/**
 * Connects the Channel's stereo in ports to its
 * stereo out ports.
 *
 * Used when deleting the only plugin left.
 */
static inline void
connect_stereo_in_to_stereo_out (
  Channel * ch)
{
  port_connect (
    ch->stereo_in->l,
    ch->stereo_out->l);
  port_connect (
    ch->stereo_in->r,
    ch->stereo_out->r);
}

/**
 * Disconnects the Channel's stereo in from the
 * Plugin's input ports.
 */
static inline void
disconnect_ch_stereo_in_from_pl (
  Channel * ch,
  Plugin * pl)
{
  int i;
  Port * in_port;

  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (in_port->identifier.type != TYPE_AUDIO)
        continue;

      if (ports_connected (
            ch->stereo_in->l,
            in_port))
        port_disconnect (
          ch->stereo_in->l,
          in_port);
      if (ports_connected (
            ch->stereo_in->r,
            in_port))
        port_disconnect (
          ch->stereo_in->r,
          in_port);
    }
}

/**
 * Connects the stereo in of the channel to the
 * inputs of the Plugin.
 */
static inline void
connect_ch_stereo_in_to_plugin (
  Channel * ch,
  Plugin * pl)
{
  int last_index, num_ports_to_connect, i;
  Port * in_port;

  num_ports_to_connect = 0;
  if (pl->descr->num_audio_ins == 1)
    {
      num_ports_to_connect = 1;
    }
  else if (pl->descr->num_audio_ins > 1)
    {
      num_ports_to_connect = 2;
    }

  last_index = 0;
  for (i = 0; i < num_ports_to_connect; i++)
    {
      for (;
           last_index < pl->num_in_ports;
           last_index++)
        {
          in_port =
            pl->in_ports[
              last_index];
          if (in_port->identifier.type == TYPE_AUDIO)
            {
              if (i == 0)
                {
                  port_connect (
                    ch->stereo_in->l,
                    in_port);
                  last_index++;
                  break;
                }
              else if (i == 1)
                {
                  port_connect (
                    ch->stereo_in->r,
                    in_port);
                  last_index++;
                  break;
                }
            }
        }
    }
}

/**
 * Connect outputs of src to inputs of dests.
 */
static inline void
connect_from_pl_to_pl (
  Plugin * src,
  Plugin * dest)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  if (src->descr->num_audio_outs == 1 &&
      dest->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      goto done1;
                    }
                }
            }
        }
done1:
      ;
    }
  else if (src->descr->num_audio_outs == 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports;
                   j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                    }
                }
              break;
            }
        }
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < dest->num_in_ports; i++)
        {
          in_port = dest->in_ports[i];

          if (in_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      goto done2;
                    }
                }
              break;
            }
        }
done2:
      ;
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (src->descr->num_audio_outs,
             dest->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     dest->num_in_ports;
                   last_index++)
                {
                  in_port =
                    dest->in_ports[last_index];
                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      last_index++;
                      ports_connected++;
                      break;
                    }
                }
              if (ports_connected ==
                  num_ports_to_connect)
                break;
            }
        }
    }
}

/**
 * Connect Plugin's outputs to the Channel's
 * stereo out ports.
 */
static inline void
connect_pl_to_stereo_out (
  Plugin * pl,
  Channel * ch)
{
  int i, last_index;
  Port * out_port;
  if (pl->descr->num_audio_outs == 1)
    {
      /* if mono find the audio out and connect to
       * both stereo out L and R */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              port_connect (
                out_port,
                ch->stereo_out->l);
              port_connect (
                out_port,
                ch->stereo_out->r);
              break;
            }
        }
    }
  else if (pl->descr->num_audio_outs > 1)
    {
      last_index = 0;

      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->identifier.type != TYPE_AUDIO)
            continue;

          if (last_index == 0)
            {
              port_connect (
                out_port,
                ch->stereo_out->l);
              last_index++;
            }
          else if (last_index == 1)
            {
              port_connect (
                out_port,
                ch->stereo_out->r);
              break;
            }
        }
    }

}

/**
 * Disconnect src from dest.
 *
 * Used when about to insert a plugin in-between.
 */
static inline void
disconnect_pl_from_pl (
  Plugin * src,
  Plugin * dest)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  if (src->descr->num_audio_outs == 1 &&
      dest->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports; j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      goto done1;
                    }
                }
            }
        }
done1:
      ;
    }
  else if (src->descr->num_audio_outs == 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so disconnect the mono out from
       * each input */
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < dest->num_in_ports;
                   j++)
                {
                  in_port = dest->in_ports[j];

                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                    }
                }
              break;
            }
        }
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins == 1)
    {
      /* disconnect multi-output channel from mono
       * by disconnecting to the first input channel
       * found */
      for (i = 0; i < dest->num_in_ports; i++)
        {
          in_port = dest->in_ports[i];

          if (in_port->identifier.type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < src->num_out_ports; j++)
                {
                  out_port = src->out_ports[j];

                  if (out_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      goto done2;
                    }
                }
              break;
            }
        }
done2:
      ;
    }
  else if (src->descr->num_audio_outs > 1 &&
           dest->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (src->descr->num_audio_outs,
             dest->descr->num_audio_ins);
      last_index = 0;
      int ports_disconnected = 0;
      for (i = 0; i < src->num_out_ports; i++)
        {
          out_port = src->out_ports[i];

          if (out_port->identifier.type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     dest->num_in_ports;
                   last_index++)
                {
                  in_port =
                    dest->in_ports[last_index];
                  if (in_port->identifier.type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      last_index++;
                      ports_disconnected++;
                      break;
                    }
                }
              if (ports_disconnected ==
                  num_ports_to_connect)
                break;
            }
        }
    }
}

/**
 * Disconnects the Plugin's output ports from
 * the Channel's stereo out.
 */
static inline void
disconnect_pl_from_stereo_out (
  Plugin * pl,
  Channel * ch)
{
  int i;
  Port * out_port;

  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];
      if (out_port->identifier.type == TYPE_AUDIO)
        {
          if (ports_connected (
                out_port, ch->stereo_out->l))
            port_disconnect (
              out_port, ch->stereo_out->l);
          if (ports_connected (
                out_port, ch->stereo_out->r))
            port_disconnect (
              out_port, ch->stereo_out->r);
        }
    }
}

/**
 * Connect ports in the case of !prev && !next.
 */
static void
connect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel
   * stereo out. disconnect it */
  disconnect_stereo_in_from_stereo_out (ch);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  connect_ch_stereo_in_to_plugin (ch, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin to stereo out */
  connect_pl_to_stereo_out (pl, ch);
}

/**
 * Connect ports in the case of !prev && next.
 */
static void
connect_no_prev_next (
  Channel * ch,
  Plugin *  pl,
  Plugin *  next_pl)
{
  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin.
   * disconnect it */
  disconnect_ch_stereo_in_from_pl (ch, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  connect_ch_stereo_in_to_plugin (ch, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin's audio outs to next
   * plugin */
  connect_from_pl_to_pl (pl, next_pl);
}

/**
 * Connect ports in the case of prev && !next.
 */
static void
connect_prev_no_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl)
{
  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to channel stereo out.
   * disconnect it */
  disconnect_pl_from_stereo_out (prev_pl, ch);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to
   * plugin */
  connect_from_pl_to_pl (prev_pl, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin output ports to stereo_out */
  connect_pl_to_stereo_out (pl, ch);
}

/**
 * Connect ports in the case of prev && next.
 */
static void
connect_prev_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl,
  Plugin *  next_pl)
{
  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to the next pl.
   * disconnect them */
  disconnect_pl_from_pl (prev_pl, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to
   * plugin */
  connect_from_pl_to_pl (prev_pl, pl);

  /* ------------------------------------------
   * Connect output ports
   * ------------------------------------------ */

  /* connect plugin's audio outs to next
   * plugin */
  connect_from_pl_to_pl (pl, next_pl);
}

/**
 * Disconnect ports in the case of !prev && !next.
 */
static void
disconnect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  disconnect_ch_stereo_in_from_pl (ch, pl);

  /* --------------------------------------
   * disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin from stereo out */
  disconnect_pl_from_stereo_out (pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to
   * channel stereo out. connect it */
  connect_stereo_in_to_stereo_out (ch);
}

/**
 * Disconnect ports in the case of !prev && next.
 */
static void
disconnect_no_prev_next (
  Channel * ch,
  Plugin *  pl,
  Plugin *  next_pl)
{
  /* -------------------------------------------
   * Disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  disconnect_ch_stereo_in_from_pl (ch, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin's audio outs from next
   * plugin */
  disconnect_pl_from_pl (pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to next
   * plugin. connect it */
  connect_ch_stereo_in_to_plugin (
    ch, next_pl);
}

/**
 * Connect ports in the case of prev && !next.
 */
static void
disconnect_prev_no_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from
   * plugin */
  disconnect_pl_from_pl (prev_pl, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin output ports from stereo
   * out */
  disconnect_pl_from_stereo_out (pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to channel
   * stereo out. connect it */
  connect_pl_to_stereo_out (prev_pl, ch);
}

/**
 * Connect ports in the case of prev && next.
 */
static void
disconnect_prev_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl,
  Plugin *  next_pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from
   * plugin */
  disconnect_pl_from_pl (prev_pl, pl);

  /* ------------------------------------------
   * Disconnect output ports
   * ------------------------------------------ */

  /* disconnect plugin's audio outs from next
   * plugin */
  disconnect_pl_from_pl (pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to the next pl.
   * connect them */
  connect_from_pl_to_pl (prev_pl, next_pl);
}

/**
 * Handles the recording logic inside the process cycle.
 */
void
channel_handle_recording (Channel * self)
{
  Region * region =
    track_get_region_at_pos (self->track,
                             &PLAYHEAD);
  /* get end position TODO snap */
  Position tmp;
  position_set_to_pos (&tmp, &PLAYHEAD);
  position_add_frames (
    &tmp,
    AUDIO_ENGINE->nframes + 1);

  if (self->type == CT_MIDI)
    {
      MidiRegion * mr = (MidiRegion *) region;

      if (region) /* if inside a region */
        {
          /* set region end pos */
          region_set_end_pos (region,
                              &tmp);
        }
      else /* if not already in a region */
        {
          /* create region */
          mr =
            midi_region_new (
              &PLAYHEAD, &tmp);
          track_add_region (
            self->track, mr, F_GEN_NAME);
        }

      /* convert MIDI data to midi notes */
#ifdef HAVE_JACK
      if (MIDI_IN_NUM_EVENTS > 0)
        {
          MidiNote * mn;
          for (int i = 0; i < MIDI_IN_NUM_EVENTS; i++)
            {
              jack_midi_event_t * event = &MIDI_IN_EVENT(i);
              jack_midi_event_get (event,
                                   AUDIO_ENGINE->port_buf,
                                   i);
              uint8_t type = event->buffer[0] & 0xf0;
              Velocity * vel;
              switch (type)
                {
                  case MIDI_CH1_NOTE_ON:
                    vel =
                      velocity_new (
                        event->buffer[2]);
                    mn =
                      midi_note_new (
                        mr, &PLAYHEAD, &tmp,
                        event->buffer[1], vel);
                    midi_region_add_midi_note (
                      mr, mn);

                    /* add to unended notes */
                    array_append (
                      mr->unended_notes,
                      mr->num_unended_notes,
                      mn);
                    break;
                  case MIDI_CH1_NOTE_OFF:
                    mn =
                      midi_region_find_unended_note (
                        mr,
                        event->buffer[1]);
                    midi_note_set_end_pos (mn,
                                           &tmp);
                    break;
                  case MIDI_CH1_CTRL_CHANGE:
                    /* TODO */
                    break;
                  default:
                          break;
                }
            } /* for loop num events */
        } /* if have midi events */
#endif
    } /* if channel type MIDI */
}

/**
 * Prepares the channel for processing.
 *
 * To be called before the main cycle each time on
 * all channels.
 */
void
channel_prepare_process (Channel * channel)
{
  Plugin * plugin;
  int i,j;

  /* clear buffers */
  if (channel->type == CT_MASTER ||
      channel->type == CT_AUDIO ||
      channel->type == CT_BUS)
    {
      port_clear_buffer (channel->stereo_in->l);
      port_clear_buffer (channel->stereo_in->r);
    }
  if (channel->type == CT_MIDI)
    {
      port_clear_buffer (channel->midi_in);
      port_clear_buffer (channel->piano_roll);
    }
  port_clear_buffer (channel->stereo_out->l);
  port_clear_buffer (channel->stereo_out->r);
  for (j = 0; j < STRIP_SIZE; j++)
    {
      plugin = channel->plugins[j];
      if (!plugin)
        continue;

      for (i = 0; i < plugin->num_in_ports;
           i++)
        {
          port_clear_buffer (plugin->in_ports[i]);
        }
      for (i = 0; i < plugin->num_out_ports;
           i++)
        {
          port_clear_buffer (plugin->out_ports[i]);
        }
      for (i = 0;
           i < plugin->num_unknown_ports; i++)
        {
          port_clear_buffer (
            plugin->unknown_ports[i]);
        }
    }
  channel->filled_stereo_in_bufs = 0;
}

void
channel_init_loaded (Channel * ch)
{
  g_message ("initing channel");

  /* fader */
  ch->fader.channel = ch;

  /* midi in / piano roll ports */
  ch->piano_roll->identifier.flags =
    PORT_FLAG_PIANO_ROLL;
  ch->midi_in->midi_events =
    midi_events_new (1);
  ch->piano_roll->midi_events =
    midi_events_new (1);

  /* routing */
  if (ch->output_pos > -1)
    ch->output =
      TRACKLIST->tracks[ch->output_pos];

  ch->widget = channel_widget_new (ch);
}

/**
 * Creates, inits, and returns a new channel with
 * given info.
 */
static inline Channel *
_create_channel (
  Track * track)
{
  Channel * channel = calloc (1, sizeof (Channel));

  channel->track = track;

  /* create ports */
  char * pll =
    g_strdup_printf ("%s stereo in L",
                     track->name);
  char * plr =
    g_strdup_printf ("%s stereo in R",
                     track->name);
  channel->stereo_in =
    stereo_ports_new (
      port_new_with_type (TYPE_AUDIO,
                          FLOW_INPUT,
                          pll),
      port_new_with_type (TYPE_AUDIO,
                          FLOW_INPUT,
                          plr));
  g_free (pll);
  g_free (plr);
  pll =
    g_strdup_printf ("%s MIDI in",
                     track->name);
  channel->midi_in =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      pll);
  channel->midi_in->midi_events =
    midi_events_new (1);
  g_free (pll);
  pll =
    g_strdup_printf ("%s Stereo out L",
                     track->name);
  plr =
    g_strdup_printf ("%s Stereo out R",
                     track->name);
  channel->stereo_out =
    stereo_ports_new (
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         pll),
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         plr));
  channel->stereo_out->l->identifier.flags |=
    PORT_FLAG_STEREO_L;
  channel->stereo_out->r->identifier.flags |=
    PORT_FLAG_STEREO_R;
  g_message ("Created stereo out ports");
  g_free (pll);
  g_free (plr);
  port_set_owner_track (
    channel->stereo_in->l,
    track);
  port_set_owner_track (
    channel->stereo_in->r,
    track);
  port_set_owner_track (
    channel->stereo_out->l,
    track);
  port_set_owner_track (
    channel->stereo_out->r,
    track);
  port_set_owner_track (
    channel->midi_in,
    track);

  /* init plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel->plugins[i] = NULL;
    }

  fader_init (&channel->fader, channel);

  /* set up piano roll port */
  char * tmp =
    g_strdup_printf ("%s Piano Roll",
                     track->name);
  channel->piano_roll =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      tmp);
  channel->piano_roll->identifier.flags =
    PORT_FLAG_PIANO_ROLL;
  channel->piano_roll->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;
  channel->piano_roll->identifier.track_pos =
    channel->track->pos;
  channel->piano_roll->track =
    channel->track;
  channel->piano_roll->midi_events =
    midi_events_new (1);

  return channel;
}

/**
 * Adds to (or subtracts from) the pan.
 */
void
channel_add_pan (void * _channel, float pan)
{
  Channel * channel = (Channel *) _channel;

  channel->fader.pan =
    CLAMP (channel->fader.pan + pan, 0.f, 1.f);
}


/**
 * Sets fader to 0.0.
 */
void
channel_reset_fader (Channel * self)
{
  fader_set_amp (&self->fader, 1.0f);
}

/**
 * Generates automatables for the channel.
 *
 * Should be called as soon as the track is
 * created.
 */
void
channel_generate_automatables (Channel * channel)
{
  g_message (
    "Generating automatables for channel %s",
    channel->track->name);

  /* generate channel automatables if necessary */
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_FADER))
    {
      array_append (
        channel->automatables,
        channel->num_automatables,
        automatable_create_fader (channel));
    }
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_PAN))
    {
      array_append (channel->automatables,
                    channel->num_automatables,
                    automatable_create_pan (channel));
    }
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_MUTE))
    {
      array_append (channel->automatables,
                    channel->num_automatables,
                    automatable_create_mute (channel));
    }
}

/**
 * Connects the channel's ports.
 */
void
channel_connect (
  Channel * ch)
{
  /* connect MIDI in port from engine's jack
   * port */
  if (AUDIO_ENGINE->midi_backend !=
        MIDI_BACKEND_DUMMY)
    {
      port_connect (AUDIO_ENGINE->midi_in,
                    ch->midi_in);
    }

  /* set default output */
  if (ch->type == CT_MASTER)
    {
      ch->output = NULL;
      ch->output_pos = -1;
      port_connect (
        ch->stereo_out->l,
        AUDIO_ENGINE->stereo_out->l);
      port_connect (
        ch->stereo_out->r,
        AUDIO_ENGINE->stereo_out->r);
    }
  else
    {
      ch->output = MIXER->master;
      ch->output_pos = MIXER->master->pos;
    }

  if (ch->type == CT_BUS ||
      ch->type == CT_AUDIO ||
      ch->type == CT_MASTER)
    {
      /* connect stereo in to stereo out */
      port_connect (ch->stereo_in->l,
                    ch->stereo_out->l);
      port_connect (ch->stereo_in->r,
                    ch->stereo_out->r);
    }

  if (ch->type != CT_MASTER)
    {
      /* connect channel out ports to master */
      port_connect (
        ch->stereo_out->l,
        MIXER->master->channel->stereo_in->l);
      port_connect (
        ch->stereo_out->r,
        MIXER->master->channel->stereo_in->r);
    }
}

/**
 * Updates the output of the Channel (where the
 * Channel routes to.
 */
void
channel_update_output (
  Channel * ch,
  Track * output)
{
  /* disconnect Channel's output from the current
   * output channel */
  port_disconnect (
    ch->stereo_out->l,
    ch->output->channel->stereo_in->l);
  port_disconnect (
    ch->stereo_out->r,
    ch->output->channel->stereo_in->r);

  /* connect Channel's output to the given output
   */
  port_connect (
    ch->stereo_out->l,
    output->channel->stereo_in->l);
  port_connect (
    ch->stereo_out->r,
    output->channel->stereo_in->r);

  ch->output = output;
  g_message ("setting output %s",
             output->name);

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_CHANNEL_OUTPUT_CHANGED,
               ch);
}

/**
 * Creates a channel of the given type with the
 * given label.
 */
Channel *
channel_new (
  ChannelType type,
  Track *     track)
{
  g_return_val_if_fail (track, NULL);

  Channel * channel =
    _create_channel (track);

  channel->type = type;

  return channel;
}

void
channel_set_phase (void * _channel, float phase)
{
  Channel * channel = (Channel *) _channel;
  channel->fader.phase = phase;

  /* FIXME use an event */
  /*if (channel->widget)*/
    /*gtk_label_set_text (channel->widget->phase_reading,*/
                        /*g_strdup_printf ("%.1f", phase));*/
}

float
channel_get_phase (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader.phase;
}

/*void*/
/*channel_set_volume (void * _channel, float volume)*/
/*{*/
  /*Channel * channel = (Channel *) _channel;*/
  /*channel->volume = volume;*/
  /*[> TODO update tooltip <]*/
  /*gtk_label_set_text (channel->widget->phase_reading,*/
                      /*g_strdup_printf ("%.1f", volume));*/
  /*g_idle_add ((GSourceFunc) gtk_widget_queue_draw,*/
              /*GTK_WIDGET (channel->widget->fader));*/
/*}*/

/*float*/
/*channel_get_volume (void * _channel)*/
/*{*/
  /*Channel * channel = (Channel *) _channel;*/
  /*return channel->volume;*/
/*}*/

/*static int*/
/*redraw_fader_asnyc (Channel * channel)*/
/*{*/
  /*gtk_widget_queue_draw (*/
    /*GTK_WIDGET (channel->widget->fader));*/

  /*return FALSE;*/
/*}*/


/*static int*/
/*redraw_pan_async (Channel * channel)*/
/*{*/
  /*gtk_widget_queue_draw (*/
    /*GTK_WIDGET (channel->widget->pan));*/

  /*return FALSE;*/
/*}*/

void
channel_set_pan (void * _channel, float pan)
{
  Channel * channel = (Channel *) _channel;
  channel->fader.pan = pan;
  /*g_idle_add ((GSourceFunc) redraw_pan_async,*/
              /*channel);*/
}

float
channel_get_pan (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader.pan;
}

float
channel_get_current_l_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader.l_port_db;
}

float
channel_get_current_r_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader.r_port_db;
}

void
channel_set_current_l_db (Channel * channel, float val)
{
  channel->fader.l_port_db = val;
}

void
channel_set_current_r_db (Channel * channel, float val)
{
  channel->fader.r_port_db = val;
}

static inline void
channel_disconnect_plugin_from_strip (
  Channel * ch,
  int pos,
  Plugin * pl)
{
  int i;

  disconnect_midi_ins (ch, pos, pl);

  Plugin * next_plugin = NULL;
  for (i = pos + 1; i < STRIP_SIZE; i++)
    {
      next_plugin = ch->plugins[i];
      if (next_plugin)
        break;
    }

  Plugin * prev_plugin = NULL;
  for (i = pos - 1; i >= 0; i--)
    {
      prev_plugin = ch->plugins[i];
      if (prev_plugin)
        break;
    }

  if (!prev_plugin && !next_plugin)
    disconnect_no_prev_no_next (ch, pl);
  else if (!prev_plugin && next_plugin)
    disconnect_no_prev_next (ch, pl, next_plugin);
  else if (prev_plugin && !next_plugin)
    disconnect_prev_no_next (ch, prev_plugin, pl);
  else if (prev_plugin && next_plugin)
    disconnect_prev_next (
      ch, prev_plugin, pl, next_plugin);
}

/**
 * Removes a plugin at pos from the channel.
 *
 * If deleting_channel is 1, the automation tracks
 * associated with he plugin are not deleted at
 * this time.
 *
 * This function will always recalculate the graph
 * in order to avoid situations where the plugin
 * might be used during processing.
 *
 * @param deleting_plugin
 * @param deleting_channel
 * @param recalc_graph Recalculate mixer graph.
 */
void
channel_remove_plugin (
  Channel * channel,
  int pos,
  int deleting_plugin,
  int deleting_channel,
  int recalc_graph)
{
  Plugin * plugin = channel->plugins[pos];
  if (plugin)
    {
      g_message ("Removing %s from %s:%d",
                 plugin->descr->name,
                 channel->track->name, pos);

      channel_disconnect_plugin_from_strip (
        channel, pos, plugin);

      /* if deleting plugin disconnect the plugin
       * entirely */
      if (deleting_plugin)
        {
          if (plugin->descr->protocol == PROT_LV2)
            {
              Lv2Plugin * lv2_plugin =
                (Lv2Plugin *)
                plugin->lv2;
              if (GTK_IS_WIDGET (
                    lv2_plugin->window))
                g_signal_handler_disconnect (
                  lv2_plugin->window,
                  lv2_plugin->delete_event_id);
              lv2_close_ui (lv2_plugin);
            }
          plugin_disconnect (plugin);
          free_later (plugin, plugin_free);
        }

      channel->plugins[pos] = NULL;

      /* if not deleting plugin (moving, etc.) just
       * disconnect its connections to the prev/
       * next slot or the channel if first/last */
      /*else*/
        /*{*/
          /*channel_disconnect_plugin_from_strip (*/
            /*channel, pos, plugin);*/
        /*}*/
    }

  if (!deleting_channel)
    automation_tracklist_update (
      &channel->track->automation_tracklist);

  if (recalc_graph)
    mixer_recalc_graph (MIXER);
}

/**
 * Adds given plugin to given position in the strip.
 *
 * The plugin must be already instantiated at this
 * point.
 *
 * @param channel The Channel.
 * @param pos The position in the strip starting
 *   from 0.
 * @param plugin The plugin to add.
 * @param confirm Confirm if an existing plugin
 *   will be overwritten.
 * @param gen_automatables Generatate plugin
 *   automatables.
 *   To be used when creating a new plugin only.
 * @param recalc_graph Recalculate mixer graph.
 *
 * @return 1 if plugin added, 0 if not.
 */
int
channel_add_plugin (
  Channel * channel,
  int       pos,
  Plugin *  plugin,
  int       confirm,
  int       gen_automatables,
  int       recalc_graph)
{
  int i;
  int prev_active = channel->track->active;
  channel->track->active = 0;

  /* confirm if another plugin exists */
  if (confirm && channel->plugins[pos])
    {
      GtkDialog * dialog =
        dialogs_get_overwrite_plugin_dialog (
          GTK_WINDOW (MAIN_WINDOW));
      int result =
        gtk_dialog_run (dialog);
      gtk_widget_destroy (GTK_WIDGET (dialog));

      /* do nothing if not accepted */
      if (result != GTK_RESPONSE_ACCEPT)
        return 0;
    }

  /* free current plugin */
  channel_remove_plugin (
    channel, pos, 1, 0, F_NO_RECALC_GRAPH);

  g_message ("Inserting %s at %s:%d",
             plugin->descr->name,
             channel->track->name, pos);
  channel->plugins[pos] = plugin;
  plugin->track = channel->track;
  plugin->track_pos = channel->track->pos;
  plugin->slot = pos;

  Plugin * next_plugin = NULL;
  for (i = pos + 1; i < STRIP_SIZE; i++)
    {
      next_plugin = channel->plugins[i];
      if (next_plugin)
        break;
    }

  Plugin * prev_plugin = NULL;
  for (i = pos - 1; i >= 0; i--)
    {
      prev_plugin = channel->plugins[i];
      if (prev_plugin)
        break;
    }

  /* ------------------------------------------
   * connect ports
   * ------------------------------------------ */

  connect_midi_ins (
    channel,
    pos,
    plugin);

  if (!prev_plugin && !next_plugin)
    connect_no_prev_no_next (
      channel,
      plugin);
  else if (!prev_plugin && next_plugin)
    connect_no_prev_next (
      channel,
      plugin,
      next_plugin);
  else if (prev_plugin && !next_plugin)
    connect_prev_no_next (
      channel,
      prev_plugin,
      plugin);
  else if (prev_plugin && next_plugin)
    connect_prev_next (
      channel,
      prev_plugin,
      plugin,
      next_plugin);

  channel->track->active= prev_active;

  if (gen_automatables)
    plugin_generate_automatables (plugin);

  EVENTS_PUSH (ET_PLUGIN_ADDED,
               plugin);

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  return 1;
}

/**
 * Returns the index of the last active slot.
 */
int
channel_get_last_active_slot_index (Channel * channel)
{
  int index = -1;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (channel->plugins[i])
        index = i;
    }
  return index;
}

/**
 * Returns the index on the mixer.
 *
 * where is this used?
 */
int
channel_get_index (Channel * channel)
{
  Track * track;
  int index = 0;
  for (int i = 0;
       i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track->type == TRACK_TYPE_MASTER ||
          track->type == TRACK_TYPE_CHORD)
        continue;

      index++;
      if (track->channel == channel)
        return index;
    }
  g_return_val_if_reached (-1);
}

/**
 * Convenience method to get the first active plugin in the channel
 */
Plugin *
channel_get_first_plugin (Channel * channel)
{
  /* find first plugin */
  Plugin * plugin = NULL;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (channel->plugins[i])
        {
          plugin = channel->plugins[i];
          break;
        }
    }
  return plugin;
}


/**
 * Connects or disconnects the MIDI editor key press
 * port to the channel's first plugin
 *
 * @param connect Connect or not.
 */
void
channel_reattach_midi_editor_manual_press_port (
  Channel * channel,
  int     connect)
{
  /* find first plugin */
  Plugin * plugin =
    channel_get_first_plugin (channel);

  if (plugin)
    {
      if (channel->type == CT_MIDI)
        {
          /* Connect/Disconnect MIDI editor manual
           * press port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->identifier.type == TYPE_EVENT &&
                  port->identifier.flow == FLOW_INPUT)
                {
                  if (connect)
                    {
                      port_connect (AUDIO_ENGINE->midi_editor_manual_press, port);
                    }
                  else
                    {
                    port_disconnect (AUDIO_ENGINE->midi_editor_manual_press, port);
                    }
                }
            }
        }
    }

  mixer_recalc_graph (MIXER);
}

/**
 * Returns the plugin's strip index on the channel
 */
int
channel_get_plugin_index (Channel * channel,
                          Plugin *  plugin)
{
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (channel->plugins[i] == plugin)
        {
          return i;
        }
    }
  g_warning ("channel_get_plugin_index: plugin not found");
  return -1;
}

/**
 * Convenience function to get the fader automatable of the channel.
 */
Automatable *
channel_get_automatable (Channel *       channel,
                         AutomatableType type)
{
  for (int i = 0; i < channel->num_automatables; i++)
    {
      Automatable * automatable =
        channel->automatables[i];

      if (type == automatable->type)
        return automatable;
    }
  return NULL;
}

/*static void*/
/*remove_automatable (*/
  /*Channel * self,*/
  /*Automatable * a)*/
/*{*/
  /*AutomationTracklist * atl =*/
    /*&self->track->automation_tracklist;*/

  /*for (int i = 0;*/
       /*i < atl->num_automation_tracks; i++)*/
    /*{*/
      /*if (atl->automation_tracks[i]->*/
            /*automatable == a)*/
        /*automation_tracklist_remove_*/

    /*}*/

  /*project_remove_automatable (a);*/
  /*automatable_free (a);*/
/*}*/

/**
 * Clones the channel recursively.
 *
 * Note the given track is not cloned.
 *
 * @param track The track to use for getting the name.
 */
Channel *
channel_clone (
  Channel * ch,
  Track *   track)
{
  g_return_val_if_fail (ch->track, NULL);

  Channel * clone =
    channel_new (ch->type, track);

  /* copy plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->plugins[i])
        {
          clone->plugins[i] =
            plugin_clone (ch->plugins[i]);
        }
    }

#define COPY_MEMBER(mem) \
  clone->mem = ch->mem

  COPY_MEMBER (type);
  clone->fader.channel = clone;
  fader_copy (&ch->fader, &clone->fader);

#undef COPY_MEMBER

  /* TODO clone port connections, same for
   * plugins */

  return clone;
}

/**
 * Disconnects the channel from the processing
 * chain.
 *
 * This should be called immediately when the
 * channel is getting deleted, and channel_free
 * should be designed to be called later after
 * an arbitrary delay.
 *
 * @param remove_pl Remove the Plugin from the
 *   Channel. Useful when deleting the channel.
 * @param recalc_graph Recalculate mixer graph.
 */
void
channel_disconnect (
  Channel * channel,
  int       remove_pl,
  int       recalc_graph)
{
  if (remove_pl)
    {
      FOREACH_STRIP
        {
          if (channel->plugins[i])
            {
              channel_remove_plugin (
                channel, i, remove_pl, 0,
                F_NO_RECALC_GRAPH);
            }
        }
    }

  port_disconnect_all (channel->stereo_in->l);
  port_disconnect_all (channel->stereo_in->r);
  port_disconnect_all (channel->midi_in);
  port_disconnect_all (channel->piano_roll);
  port_disconnect_all (channel->stereo_out->l);
  port_disconnect_all (channel->stereo_out->r);

  if (recalc_graph)
    mixer_recalc_graph (MIXER);
}

/**
 * Frees the channel.
 *
 * Channels should never be free'd by themselves
 * in normal circumstances. Use track_free to
 * free them.
 */
void
channel_free (Channel * channel)
{
  g_return_if_fail (channel);

  port_free (channel->stereo_in->l);
  port_free (channel->stereo_in->r);
  port_free (channel->midi_in);
  port_free (channel->piano_roll);
  port_free (channel->stereo_out->l);
  port_free (channel->stereo_out->r);

  /* remove plugins */
  /*Plugin * pl;*/
  /*for (int i = 0; i < STRIP_SIZE; i++)*/
    /*{*/
      /*pl = channel->plugins[i];*/
      /*g_message ("------attempting to free pl %d",*/
                 /*i);*/
      /*if (!pl)*/
        /*continue;*/
      /*g_message ("freeing");*/

      /*plugin_free (pl);*/
    /*}*/


  Automatable * a;
  FOREACH_AUTOMATABLE (channel)
    {
      a = channel->automatables[i];
      /*remove_automatable (channel, a);*/
      automatable_free (a);
    }

  if (channel->widget)
    gtk_widget_destroy (
      GTK_WIDGET (channel->widget));
}

