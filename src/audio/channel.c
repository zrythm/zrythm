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
#include "audio/mixer.h"
#include "audio/pan.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/connections.h"
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
      if (in_port->type == TYPE_EVENT &&
          in_port->flow == FLOW_INPUT)
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
 * Connect ports in the case of !prev && !next.
 */
static void
connect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  int i, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel
   * stereo out. disconnect it */
  port_disconnect (
    ch->stereo_in->l,
    ch->stereo_out->l);
  port_disconnect (
    ch->stereo_in->r,
    ch->stereo_out->r);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
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
          if (in_port->type == TYPE_AUDIO)
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

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  num_ports_to_connect = 0;
  if (pl->descr->num_audio_outs > 0)
    {
      num_ports_to_connect = 2;
    }

  last_index = 0;
  for (i = 0; i < num_ports_to_connect; i++)
    {
      for (;
           last_index < pl->num_out_ports;
           last_index++)
        {
          out_port =
            pl->out_ports[last_index];

          if (out_port->type == TYPE_AUDIO)
            {
              if (i == 0)
                {
                  port_connect (
                    out_port,
                    ch->stereo_out->l);

                  /* if mono, also connect right
                   * and bump i */
                  if (pl->descr->
                        num_audio_outs == 1)
                    {
                      port_connect (
                        out_port,
                        ch->stereo_out->r);
                      i++;
                    }

                  last_index++;
                  break;
                }
              else if (i == 1)
                {
                  port_connect (
                    out_port,
                    ch->stereo_out->r);
                  last_index++;
                  break;
                }
            }
        }
    }
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
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin.
   * disconnect it */
  for (i = 0; i < next_pl->num_in_ports; i++)
    {
      in_port = next_pl->in_ports[i];
      if (in_port->type == TYPE_AUDIO)
        {
          if (ports_connected (
                ch->stereo_in->l, in_port))
            port_disconnect (
              ch->stereo_in->l,
              in_port);
          if (ports_connected (
                ch->stereo_in->r, in_port))
            port_disconnect (
              ch->stereo_in->r,
              in_port);
        }
    }

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
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
          if (in_port->type == TYPE_AUDIO)
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

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin's audio outs to next
   * plugin */
  if (pl->descr->num_audio_outs == 1 &&
      next_pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports; j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs == 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports;
                   j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < next_pl->num_in_ports; i++)
        {
          in_port = next_pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < pl->num_out_ports; j++)
                {
                  out_port = pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (pl->descr->num_audio_outs,
             next_pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     next_pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    next_pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
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
 * Connect ports in the case of prev && !next.
 */
static void
connect_prev_no_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to channel stereo out.
   * disconnect it */
  for (i = 0; i < prev_pl->num_out_ports; i++)
    {
      out_port = prev_pl->out_ports[i];
      if (out_port->type == TYPE_AUDIO)
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

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to
   * plugin */
  if (prev_pl->descr->num_audio_outs == 1 &&
      pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports; j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs == 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* prev plugin is mono and this plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports;
                   j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < prev_pl->num_out_ports; j++)
                {
                  out_port = prev_pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs the prev
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (prev_pl->descr->num_audio_outs,
             pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index < pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
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


  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin output ports to stereo_out */
  if (pl->descr->num_audio_outs == 1)
    {
      /* if mono find the audio out and connect to
       * both stereo out L and R */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
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

          if (out_port->type != TYPE_AUDIO)
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
 * Connect ports in the case of prev && next.
 */
static void
connect_prev_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl,
  Plugin *  next_pl)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to the next pl.
   * disconnect them */
  for (i = 0; i < prev_pl->num_out_ports; i++)
    {
      out_port = prev_pl->out_ports[i];

      if (out_port->type != TYPE_AUDIO)
        continue;

      for (j = 0; j < next_pl->num_in_ports; j++)
        {
          in_port = next_pl->in_ports[j];

          if (in_port->type != TYPE_AUDIO)
            continue;

          if (ports_connected (
                out_port, in_port))
            port_disconnect (
              out_port, in_port);
        }
    }

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to
   * plugin */
  if (prev_pl->descr->num_audio_outs == 1 &&
      pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports; j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs == 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* prev plugin is mono and this plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports;
                   j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < prev_pl->num_out_ports; j++)
                {
                  out_port = prev_pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs the prev
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (prev_pl->descr->num_audio_outs,
             pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index < pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
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

  /* ------------------------------------------
   * Connect output ports
   * ------------------------------------------ */

  /* connect plugin's audio outs to next
   * plugin */
  if (pl->descr->num_audio_outs == 1 &&
      next_pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports; j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      goto done3;
                    }
                }
            }
        }
done3:
      ;
    }
  else if (pl->descr->num_audio_outs == 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports;
                   j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < next_pl->num_in_ports; i++)
        {
          in_port = next_pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < pl->num_out_ports; j++)
                {
                  out_port = pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      goto done4;
                    }
                }
              break;
            }
        }
done4:
      ;
    }
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (pl->descr->num_audio_outs,
             next_pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     next_pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    next_pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
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
      if (in_port->type == TYPE_EVENT &&
          in_port->flow == FLOW_INPUT)
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
 * Disconnect ports in the case of !prev && !next.
 */
static void
disconnect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  int i;
  Port * in_port, * out_port;

  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (in_port->type != TYPE_AUDIO)
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

  /* --------------------------------------
   * disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin from stereo out */
  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];

      if (out_port->type != TYPE_AUDIO)
        continue;

      if (ports_connected (
            out_port,
            ch->stereo_out->l))
        port_disconnect (
          out_port,
          ch->stereo_out->l);
      if (ports_connected (
            out_port,
            ch->stereo_out->r))
        port_disconnect (
          out_port,
          ch->stereo_out->r);
    }

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to
   * channel stereo out. connect it */
  port_connect (
    ch->stereo_in->l,
    ch->stereo_out->l);
  port_connect (
    ch->stereo_in->r,
    ch->stereo_out->r);
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
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  /* -------------------------------------------
   * Disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (in_port->type != TYPE_AUDIO)
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

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin's audio outs from next
   * plugin */
  if (pl->descr->num_audio_outs == 1 &&
      next_pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports; j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs == 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so disconnect the mono out from
       * each input */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports;
                   j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins == 1)
    {
      /* disconnect multi-output channel from mono
       * by disconnecting to the first input channel
       * found */
      for (i = 0; i < next_pl->num_in_ports; i++)
        {
          in_port = next_pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < pl->num_out_ports; j++)
                {
                  out_port = pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (pl->descr->num_audio_outs,
             next_pl->descr->num_audio_ins);
      last_index = 0;
      int ports_disconnected = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     next_pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    next_pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
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

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to next
   * plugin. connect it */
  num_ports_to_connect = 0;
  if (next_pl->descr->num_audio_ins == 1)
    {
      num_ports_to_connect = 1;
    }
  else if (next_pl->descr->num_audio_ins > 1)
    {
      num_ports_to_connect = 2;
    }

  last_index = 0;
  for (i = 0; i < num_ports_to_connect; i++)
    {
      for (;
           last_index < next_pl->num_in_ports;
           last_index++)
        {
          in_port =
            next_pl->in_ports[
              last_index];
          if (in_port->type == TYPE_AUDIO)
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
 * Connect ports in the case of prev && !next.
 */
static void
disconnect_prev_no_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;


  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from
   * plugin */
  if (prev_pl->descr->num_audio_outs == 1 &&
      pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports; j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs == 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* prev plugin is mono and this plugin is
       * not mono, so disconnect the mono out from
       * each input */
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports;
                   j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins == 1)
    {
      /* disconnect multi-output channel into mono
       * by disconnecting from the first input
       * channel found */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < prev_pl->num_out_ports; j++)
                {
                  out_port = prev_pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* disconnect to as many audio outs the prev
       * plugin has, or until we can't disconnect
       * anymore */
      num_ports_to_connect =
        MIN (prev_pl->descr->num_audio_outs,
             pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index < pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
                    {
                      port_disconnect (
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


  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin output ports from stereo
   * out */
  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];

      if (out_port->type == TYPE_AUDIO)
        {
          if (ports_connected (
            out_port,
            ch->stereo_out->l))
            port_disconnect (
              out_port,
              ch->stereo_out->l);
          if (ports_connected (
            out_port,
            ch->stereo_out->r))
            port_disconnect (
              out_port,
              ch->stereo_out->r);
        }
    }

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to channel
   * stereo out. connect it */
  if (prev_pl->descr->num_audio_outs == 1)
    {
      /* if mono find the audio out and connect to
       * both stereo out L and R */
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1)
    {
      last_index = 0;

      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type != TYPE_AUDIO)
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
 * Connect ports in the case of prev && next.
 */
static void
disconnect_prev_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl,
  Plugin *  next_pl)
{
  int i, j, last_index, num_ports_to_connect;
  Port * in_port, * out_port;

  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from
   * plugin */
  if (prev_pl->descr->num_audio_outs == 1 &&
      pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports; j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs == 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* prev plugin is mono and this plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0; j < pl->num_in_ports;
                   j++)
                {
                  in_port = pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < prev_pl->num_out_ports; j++)
                {
                  out_port = prev_pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
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
  else if (prev_pl->descr->num_audio_outs > 1 &&
           pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs the prev
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (prev_pl->descr->num_audio_outs,
             pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < prev_pl->num_out_ports; i++)
        {
          out_port = prev_pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index < pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
                    {
                      port_disconnect (
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

  /* ------------------------------------------
   * Disconnect output ports
   * ------------------------------------------ */

  /* disconnect plugin's audio outs from next
   * plugin */
  if (pl->descr->num_audio_outs == 1 &&
      next_pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports; j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      goto done3;
                    }
                }
            }
        }
done3:
      ;
    }
  else if (pl->descr->num_audio_outs == 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports;
                   j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < next_pl->num_in_ports; i++)
        {
          in_port = next_pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < pl->num_out_ports; j++)
                {
                  out_port = pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
                    {
                      port_disconnect (
                        out_port,
                        in_port);
                      goto done4;
                    }
                }
              break;
            }
        }
done4:
      ;
    }
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (pl->descr->num_audio_outs,
             next_pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     next_pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    next_pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
                    {
                      port_disconnect (
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

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to the next pl.
   * connect them */
  if (pl->descr->num_audio_outs == 1 &&
      next_pl->descr->num_audio_ins == 1)
    {
      last_index = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports; j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      goto done5;
                    }
                }
            }
        }
done5:
      ;
    }
  else if (pl->descr->num_audio_outs == 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* plugin is mono and next plugin is
       * not mono, so connect the mono out to
       * each input */
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < next_pl->num_in_ports;
                   j++)
                {
                  in_port = next_pl->in_ports[j];

                  if (in_port->type == TYPE_AUDIO)
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
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins == 1)
    {
      /* connect multi-output channel into mono by
       * only connecting to the first input channel
       * found */
      for (i = 0; i < next_pl->num_in_ports; i++)
        {
          in_port = next_pl->in_ports[i];

          if (in_port->type == TYPE_AUDIO)
            {
              for (j = 0;
                   j < pl->num_out_ports; j++)
                {
                  out_port = pl->out_ports[j];

                  if (out_port->type == TYPE_AUDIO)
                    {
                      port_connect (
                        out_port,
                        in_port);
                      goto done6;
                    }
                }
              break;
            }
        }
done6:
      ;
    }
  else if (pl->descr->num_audio_outs > 1 &&
           next_pl->descr->num_audio_ins > 1)
    {
      /* connect to as many audio outs this
       * plugin has, or until we can't connect
       * anymore */
      num_ports_to_connect =
        MIN (pl->descr->num_audio_outs,
             next_pl->descr->num_audio_ins);
      last_index = 0;
      int ports_connected = 0;
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];

          if (out_port->type == TYPE_AUDIO)
            {
              for (;
                   last_index <
                     next_pl->num_in_ports;
                   last_index++)
                {
                  in_port =
                    next_pl->in_ports[last_index];
                  if (in_port->type == TYPE_AUDIO)
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
              self->track, &PLAYHEAD, &tmp,
              F_ADD_TO_PROJ);
          region = (Region *) mr;
          track_add_region (self->track,
                            region);
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
                        event->buffer[1], vel,
                        F_ADD_TO_PROJ);
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
  /* plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    ch->plugins[i] =
      project_get_plugin (ch->plugin_ids[i]);

  /* fader */
  ch->fader.channel = ch;

  /* stereo in/out ports */
  ch->stereo_in->l =
    project_get_port (ch->stereo_in->l_id);
  ch->stereo_in->r =
    project_get_port (ch->stereo_in->r_id);
  ch->stereo_out->l =
    project_get_port (ch->stereo_out->l_id);
  ch->stereo_out->r =
    project_get_port (ch->stereo_out->r_id);

  /* midi in / piano roll ports */
  ch->midi_in =
    project_get_port (ch->midi_in_id);
  ch->piano_roll =
    project_get_port (ch->piano_roll_id);
  ch->piano_roll->flags = PORT_FLAG_PIANO_ROLL;
  ch->midi_in->midi_events =
    midi_events_new (1);
  ch->piano_roll->midi_events =
    midi_events_new (1);

  /* routing */
  if (ch->output_id > -1)
    ch->output =
      project_get_channel (ch->output_id);

  /* automatables */
  for (int i = 0; i < ch->num_automatables; i++)
    ch->automatables[i] =
      project_get_automatable (
        ch->automatable_ids[i]);

  /* track */
  ch->track =
    project_get_track (ch->track_id);

  /*zix_sem_init (&ch->processed_sem, 1);*/
  /*zix_sem_init (&ch->start_processing_sem, 0);*/

  ch->widget = channel_widget_new (ch);
}

/**
 * Creates, inits, and returns a new channel with
 * given info.
 *
 * @param add_to_project Add to project or not.
 */
static inline Channel *
_create_channel (
  char * name,
  int    add_to_project)
{
  Channel * channel = calloc (1, sizeof (Channel));

  /* create ports */
  char * pll =
    g_strdup_printf ("%s stereo in L",
                     name);
  char * plr =
    g_strdup_printf ("%s stereo in R",
                     name);
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
                     name);
  channel->midi_in =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      pll);
  channel->midi_in_id = channel->midi_in->id;
  channel->midi_in->midi_events =
    midi_events_new (1);
  g_free (pll);
  pll =
    g_strdup_printf ("%s Stereo out L",
                     name);
  plr =
    g_strdup_printf ("%s Stereo out R",
                     name);
  channel->stereo_out =
    stereo_ports_new (
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         pll),
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         plr));
  channel->stereo_out->l->flags |=
    PORT_FLAG_STEREO_L;
  channel->stereo_out->r->flags |=
    PORT_FLAG_STEREO_R;
  g_message ("Created stereo out ports");
  g_free (pll);
  g_free (plr);
  port_set_owner_channel (channel->stereo_in->l,
                          channel);
  port_set_owner_channel (channel->stereo_in->r,
                          channel);
  port_set_owner_channel (channel->stereo_out->l,
                          channel);
  port_set_owner_channel (channel->stereo_out->r,
                          channel);
  port_set_owner_channel (channel->midi_in,
                          channel);

  /* init plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel->plugins[i] = NULL;
      channel->plugin_ids[i] = -1;
    }

  fader_init (&channel->fader, channel);

  /* init semaphores */
  /*zix_sem_init (&channel->processed_sem, 1);*/
  /*zix_sem_init (&channel->start_processing_sem, 0);*/

  /* set up piano roll port */
  char * tmp =
    g_strdup_printf ("%s Piano Roll",
                     name);
  channel->piano_roll =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      tmp);
  channel->piano_roll->flags = PORT_FLAG_PIANO_ROLL;
  channel->piano_roll_id =
    channel->piano_roll->id;
  channel->piano_roll->is_piano_roll = 1;
  channel->piano_roll->owner_backend = 0;
  channel->piano_roll->owner_ch = channel;
  channel->piano_roll->midi_events =
    midi_events_new (1);

  channel->visible = 1;

  if (add_to_project)
    project_add_channel (channel);

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
      channel->automatable_ids[
        channel->num_automatables - 1] =
          channel->automatables[
            channel->num_automatables - 1]->id;
    }
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_PAN))
    {
      array_append (channel->automatables,
                    channel->num_automatables,
                    automatable_create_pan (channel));
      channel->automatable_ids[
        channel->num_automatables - 1] =
          channel->automatables[
            channel->num_automatables - 1]->id;
    }
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_MUTE))
    {
      array_append (channel->automatables,
                    channel->num_automatables,
                    automatable_create_mute (channel));
      channel->automatable_ids[
        channel->num_automatables - 1] =
          channel->automatables[
            channel->num_automatables - 1]->id;
    }
}

/**
 * Used when loading projects.
 */
Channel *
channel_get_or_create_blank (int id)
{
  if (MIXER->channels[id])
    {
      return MIXER->channels[id];
    }

  Channel * channel = calloc (1, sizeof (Channel));

  channel->id = id;

  /* thread related */
  /*setup_thread (channel);*/

  MIXER->channels[id] = channel;
  MIXER->num_channels++;

  g_message ("[channel_new] Creating blank channel %d", id);

  return channel;
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
      ch->output_id = -1;
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
      ch->output_id = MIXER->master->id;
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
      port_connect (ch->stereo_out->l,
                    MIXER->master->stereo_in->l);
      port_connect (ch->stereo_out->r,
                    MIXER->master->stereo_in->r);
    }
}

/**
 * Creates a channel of the given type with the
 * given label.
 *
 * This should not be creating a track. A track
 * should be created from an existing channel.
 *
 * @param add_to_project Whether the channel should
 *   be added to the project or not. This should be
 *   true unless the channel will be transient (e.g.
 *   in an undo action).
 */
Channel *
channel_new (
  ChannelType type,
  char *      label,
  int         add_to_project)
{
  g_warn_if_fail (label);

  Channel * channel =
    _create_channel (label,
                     add_to_project);

  channel->type = type;

  g_message ("Created channel %s of type %i",
             label, type);

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
 */
void
channel_remove_plugin (
  Channel * channel,
  int pos,
  int deleting_plugin,
  int deleting_channel)
{
  Plugin * plugin = channel->plugins[pos];
  if (plugin)
    {
      g_message ("Removing %s from %s:%d",
                 plugin->descr->name,
                 channel->track->name, pos);
      channel->plugins[pos] = NULL;
      channel->plugin_ids[pos] = -1;

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
  int prev_enabled = channel->enabled;
  channel->enabled = 0;

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
    channel, pos, 1, 0);

  g_message ("Inserting %s at %s:%d", plugin->descr->name,
             channel->track->name, pos);
  channel->plugins[pos] = plugin;
  channel->plugin_ids[pos] = plugin->id;
  plugin->channel = channel;
  plugin->channel_id = channel->id;
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

  channel->enabled = prev_enabled;

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
 */
int
channel_get_index (Channel * channel)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      if (MIXER->channels[i] == channel)
        return i;
    }
  g_error ("Channel index for %s not found", channel->track->name);
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
 * Connects or disconnects the MIDI editor key press port to the channel's
 * first plugin
 */
void
channel_reattach_midi_editor_manual_press_port (Channel * channel,
                                                int     connect)
{
  /* find first plugin */
  Plugin * plugin = channel_get_first_plugin (channel);

  if (plugin)
    {
      if (channel->type == CT_MIDI)
        {
          /* Connect/Disconnect MIDI editor manual press port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
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
      Automatable * automatable = channel->automatables[i];

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
 */
Channel *
channel_clone (
  Channel * ch)
{
  g_return_val_if_fail (ch->track, NULL);

  Channel * clone =
    channel_new (ch->type, ch->track->name,
                 F_NO_ADD_TO_PROJ);

  /* copy plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->plugins[i])
        {
          clone->plugins[i] =
            plugin_clone (ch->plugins[i]);
          clone->plugin_ids[i] =
            clone->plugins[i]->id;
        }
    }

#define COPY_MEMBER(mem) \
  clone->mem = ch->mem

  COPY_MEMBER (type);
  clone->fader.channel = clone;
  fader_copy (&ch->fader, &clone->fader);
  COPY_MEMBER (enabled);
  COPY_MEMBER (visible);

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
 */
void
channel_disconnect (Channel * channel)
{
  FOREACH_STRIP
    {
      if (channel->plugins[i])
        {
          channel_remove_plugin (channel, i, 1, 0);
        }
    }
  port_disconnect_all (channel->stereo_in->l);
  port_disconnect_all (channel->stereo_in->r);
  port_disconnect_all (channel->midi_in);
  port_disconnect_all (channel->piano_roll);
  port_disconnect_all (channel->stereo_out->l);
  port_disconnect_all (channel->stereo_out->r);

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
  project_remove_channel (channel);

  project_remove_port (channel->stereo_in->l);
  port_free (channel->stereo_in->l);
  project_remove_port (channel->stereo_in->r);
  port_free (channel->stereo_in->r);
  project_remove_port (channel->midi_in);
  port_free (channel->midi_in);
  project_remove_port (channel->piano_roll);
  port_free (channel->piano_roll);
  project_remove_port (channel->stereo_out->l);
  port_free (channel->stereo_out->l);
  project_remove_port (channel->stereo_out->r);
  port_free (channel->stereo_out->r);

  Automatable * a;
  FOREACH_AUTOMATABLE (channel)
    {
      a = channel->automatables[i];
      /*remove_automatable (channel, a);*/
      project_remove_automatable (a);
      automatable_free (a);
    }

  if (channel->widget)
    gtk_widget_destroy (
      GTK_WIDGET (channel->widget));
}

