/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex and zrythm dot org>
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
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/modulator.h"
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
#include "utils/stoat.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Disconnect stereo in ports from the fader.
 *
 * Used when there is no plugin in the channel.
 */
static inline void
disconnect_ch_input_from_fader (
  Channel * ch)
{
  switch (ch->track->in_signal_type)
    {
    case TYPE_AUDIO:
      port_disconnect (
        ch->stereo_in->l,
        ch->prefader.stereo_in->l);
      port_disconnect (
        ch->stereo_in->r,
        ch->prefader.stereo_in->r);
      break;
    case TYPE_EVENT:
      port_disconnect (ch->midi_in, ch->midi_out);
      if (track_has_piano_roll (ch->track))
        port_disconnect (
          ch->piano_roll, ch->midi_out);
      break;
    default:
      break;
    }
}

/**
 * Connects the Channel's stereo in ports to its
 * fader in ports.
 *
 * Used when deleting the only plugin left.
 */
static inline void
connect_ch_input_to_fader (
  Channel * ch)
{
  /* connect only if signals match */
  if (ch->track->in_signal_type == TYPE_AUDIO &&
      ch->track->out_signal_type == TYPE_AUDIO)
    {
      port_connect (
        ch->stereo_in->l,
        ch->prefader.stereo_in->l, 1);
      port_connect (
        ch->stereo_in->r,
        ch->prefader.stereo_in->r, 1);
    }
  if (ch->track->in_signal_type == TYPE_EVENT &&
      ch->track->out_signal_type == TYPE_EVENT)
    {
      port_connect (
        ch->midi_in,
        ch->prefader.midi_in, 1);
      if (track_has_piano_roll (ch->track))
        port_connect (
          ch->piano_roll, ch->prefader.midi_in, 1);
    }
}

/**
 * Disconnects the Channel's stereo in from the
 * Plugin's input ports.
 */
static inline void
disconnect_ch_input_from_pl (
  Channel * ch,
  Plugin * pl)
{
  int i;
  Port * in_port;
  PortType type = ch->track->in_signal_type;

  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (type == TYPE_AUDIO)
        {
          if (in_port->identifier.type !=
                TYPE_AUDIO)
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
      else if (type == TYPE_EVENT)
        {
          if (in_port->identifier.type !=
                TYPE_EVENT)
            continue;

          if (ports_connected (
                ch->midi_in, in_port))
            port_disconnect (
              ch->midi_in, in_port);
          if (ports_connected (
                ch->piano_roll, in_port))
            port_disconnect (
              ch->piano_roll, in_port);
        }
    }
}

/**
 * Connects the stereo in of the channel to the
 * inputs of the Plugin.
 */
static inline void
connect_ch_input_to_pl (
  Channel * ch,
  Plugin * pl)
{
  int last_index, num_ports_to_connect, i;
  Port * in_port;

  if (ch->track->in_signal_type ==
        TYPE_EVENT)
    {
      /* Connect MIDI port to the plugin */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];
          if (in_port->identifier.type ==
                TYPE_EVENT &&
              in_port->identifier.flow ==
                FLOW_INPUT)
            {
              port_connect (
                ch->midi_in, in_port, 1);

              if (track_has_piano_roll (
                    ch->track))
                port_connect (
                  ch->piano_roll, in_port, 1);
            }
        }
    }
  else if (ch->track->in_signal_type ==
             TYPE_AUDIO)
    {
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
              if (in_port->identifier.type ==
                    TYPE_AUDIO)
                {
                  if (i == 0)
                    {
                      port_connect (
                        ch->stereo_in->l,
                        in_port, 1);
                      last_index++;
                      break;
                    }
                  else if (i == 1)
                    {
                      port_connect (
                        ch->stereo_in->r,
                        in_port, 1);
                      last_index++;
                      break;
                    }
                }
            }
        }
    }
}

/**
 * Connect outputs of src to inputs of dests.
 */
static inline void
connect_pl_to_pl (
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
                        in_port, 1);
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
                        in_port, 1);
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
                        in_port, 1);
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
                        in_port, 1);
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

  /* connect prev midi outs to next midi ins */
  /* this connects only one midi out to all of the
   * midi ins of the next plugin */
  for (i = 0; i < src->num_out_ports; i++)
    {
      out_port = src->out_ports[i];

      if (out_port->identifier.type == TYPE_EVENT)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->identifier.type == TYPE_EVENT)
                {
                  port_connect (
                    out_port,
                    in_port, 1);
                }
            }
          break;
        }
    }
}

/**
 * Connect Plugin's outputs to the Channel's
 * fader in ports.
 */
static inline void
connect_pl_to_fader (
  Plugin * pl,
  Channel * ch)
{
  int i, last_index;
  Port * out_port;
  PortType type = ch->track->out_signal_type;

  if (type == TYPE_EVENT)
    {
      for (i = 0; i < pl->num_out_ports; i++)
        {
          out_port = pl->out_ports[i];
          if (out_port->identifier.type ==
                TYPE_EVENT &&
              out_port->identifier.flow ==
                FLOW_OUTPUT)
            {
              port_connect (
                out_port, ch->midi_out, 1);
            }
        }
    }
  else if (type == TYPE_AUDIO)
    {
      if (pl->descr->num_audio_outs == 1)
        {
          /* if mono find the audio out and connect to
           * both stereo out L and R */
          for (i = 0; i < pl->num_out_ports; i++)
            {
              out_port = pl->out_ports[i];

              if (out_port->identifier.type ==
                    TYPE_AUDIO)
                {
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->l, 1);
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->r, 1);
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

              if (out_port->identifier.type !=
                    TYPE_AUDIO)
                continue;

              if (last_index == 0)
                {
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->l, 1);
                  last_index++;
                }
              else if (last_index == 1)
                {
                  port_connect (
                    out_port,
                    ch->prefader.stereo_in->r, 1);
                  break;
                }
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

  /* disconnect MIDI connections */
  for (i = 0; i < src->num_out_ports; i++)
    {
      out_port = src->out_ports[i];

      if (out_port->identifier.type == TYPE_EVENT)
        {
          for (j = 0;
               j < dest->num_in_ports; j++)
            {
              in_port = dest->in_ports[j];

              if (in_port->identifier.type ==
                    TYPE_EVENT)
                {
                  port_disconnect (
                    out_port,
                    in_port);
                }
            }
        }
    }
}

/**
 * Disconnects the Plugin's output ports from
 * the Channel's fader out.
 */
static inline void
disconnect_pl_from_fader (
  Plugin * pl,
  Channel * ch)
{
  int i;
  Port * out_port;
  PortType type = ch->track->out_signal_type;

  for (i = 0; i < pl->num_out_ports; i++)
    {
      out_port = pl->out_ports[i];
      if (type == TYPE_AUDIO &&
          out_port->identifier.type == TYPE_AUDIO)
        {
          if (ports_connected (
                out_port,
                ch->prefader.stereo_in->l))
            port_disconnect (
              out_port,
              ch->prefader.stereo_in->l);
          if (ports_connected (
                out_port,
                ch->prefader.stereo_in->r))
            port_disconnect (
              out_port,
              ch->prefader.stereo_in->r);
        }
      else if (type == TYPE_EVENT &&
               out_port->identifier.type ==
                 TYPE_EVENT)
        {
          if (ports_connected (
                out_port,
                ch->prefader.midi_in))
            port_disconnect (
              out_port,
              ch->prefader.midi_in);
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
  disconnect_ch_input_from_fader (ch);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  connect_ch_input_to_pl (ch, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin to stereo out */
  connect_pl_to_fader (pl, ch);
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
  disconnect_ch_input_from_pl (ch, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  connect_ch_input_to_pl (ch, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin's audio outs to next
   * plugin */
  connect_pl_to_pl (pl, next_pl);
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
  disconnect_pl_from_fader (prev_pl, ch);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's outs to
   * plugin */
  connect_pl_to_pl (prev_pl, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin output ports to stereo_out */
  connect_pl_to_fader (pl, ch);
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
  connect_pl_to_pl (prev_pl, pl);

  /* ------------------------------------------
   * Connect output ports
   * ------------------------------------------ */

  /* connect plugin's audio outs to next
   * plugin */
  connect_pl_to_pl (pl, next_pl);
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
  disconnect_ch_input_from_pl (ch, pl);

  /* --------------------------------------
   * disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin from stereo out */
  disconnect_pl_from_fader (pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to
   * channel stereo out. connect it */
  connect_ch_input_to_fader (ch);
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
  disconnect_ch_input_from_pl (ch, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin's midi & audio outs from next
   * plugin */
  disconnect_pl_from_pl (pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to next
   * plugin. connect it */
  connect_ch_input_to_pl (ch, next_pl);
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
  disconnect_pl_from_fader (pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to channel
   * stereo out. connect it */
  connect_pl_to_fader (prev_pl, ch);
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
  connect_pl_to_pl (prev_pl, next_pl);
}

/**
 * Handles the recording logic inside the process
 * cycle.
 *
 * The MidiEvents are already dequeued at this
 * point.
 */
REALTIME
void
channel_handle_recording (Channel * self)
{
  Region * region =
    track_get_region_at_pos (
      self->track, PLAYHEAD);
  /* get end position TODO snap */
  Position tmp;
  position_set_to_pos (&tmp, PLAYHEAD);
  position_add_frames (
    &tmp,
    AUDIO_ENGINE->nframes + 1);

  if (track_has_piano_roll (self->track))
    {
      MidiRegion * mr = (MidiRegion *) region;

      if (region) /* if inside a region */
        {
          /* set region end pos */
          region_set_end_pos (
            region, &tmp, AO_UPDATE_ALL);
        }
      else /* if not already in a region */
        {
          /* create region */
          mr =
            midi_region_new (
              PLAYHEAD, &tmp, 1);
          track_add_region (
            self->track, mr, 0, F_GEN_NAME,
            F_GEN_WIDGET);
        }

      /* convert MIDI data to midi notes */
      MidiEvents * midi_events =
        AUDIO_ENGINE->midi_in->midi_events;
      if (midi_events->num_events > 0)
        {
          MidiNote * mn;
          for (int i = 0;
               i < midi_events->num_events; i++)
            {
              MidiEvent * ev =
                & midi_events->events[i];

              switch (ev->type)
                {
                  case MIDI_EVENT_TYPE_NOTE_ON:
                    mn =
                      midi_note_new (
                        mr, PLAYHEAD, &tmp,
                        ev->note_pitch,
                        ev->velocity, 1);
                    midi_region_add_midi_note (
                      mr, mn);

                    /* add to unended notes */
                    array_append (
                      mr->unended_notes,
                      mr->num_unended_notes,
                      mn);
                    break;
                  case MIDI_EVENT_TYPE_NOTE_OFF:
                    mn =
                      midi_region_find_unended_note (
                        mr, ev->note_pitch);
                    midi_note_set_end_pos (
                      mn, &tmp, AO_UPDATE_ALL);
                    break;
                  default:
                    /* TODO */
                    break;
                }
            } /* for loop num events */
        } /* if have midi events */
    } /* if channel type MIDI */
}

/**
 * Prepares the channel for processing.
 *
 * To be called before the main cycle each time on
 * all channels.
 */
void
channel_prepare_process (Channel * self)
{
  Plugin * plugin;
  int i,j;
  PortType in_type =
    self->track->in_signal_type;
  PortType out_type =
    self->track->out_signal_type;

  /* clear buffers */
  if (in_type == TYPE_AUDIO)
    {
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
    }
  else if (in_type == TYPE_EVENT)
    {
      port_clear_buffer (self->midi_in);
      if (track_has_piano_roll (self->track))
        port_clear_buffer (self->piano_roll);
    }

  passthrough_processor_clear_buffers (
    &self->prefader);
  fader_clear_buffers (
    &self->fader);

  if (out_type == TYPE_AUDIO)
    {
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
    }
  else if (out_type == TYPE_EVENT)
    {
      port_clear_buffer (self->midi_out);
    }

  for (j = 0; j < STRIP_SIZE; j++)
    {
      plugin = self->plugins[j];
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
  self->filled_stereo_in_bufs = 0;
}

void
channel_init_loaded (Channel * ch)
{
  int i;

  g_message ("initing channel");
  g_warn_if_fail (ch->track);

  /* fader */
  ch->prefader.channel = ch;
  ch->fader.channel = ch;

  /* midi in / piano roll ports */
  ch->piano_roll->identifier.flags =
    PORT_FLAG_PIANO_ROLL;
  ch->piano_roll->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;
  ch->midi_in->midi_events =
    midi_events_new (
      ch->midi_in);
  ch->midi_out->midi_events =
    midi_events_new (
      ch->midi_out);
  ch->piano_roll->midi_events =
    midi_events_new (
      ch->piano_roll);
  ch->midi_in->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;
  ch->midi_out->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;

  /* routing */
  if (ch->output_pos > -1)
    ch->output =
      TRACKLIST->tracks[ch->output_pos];

  /* if aggregated plugins not already expanded */
  if (ch->num_aggregated_plugins &&
      !ch->plugins[0])
    {
      /* expand them */
      Plugin * pl;
      for (i = 0; i < ch->num_aggregated_plugins;
           i++)
        {
          pl = ch->aggregated_plugins[i];
          ch->plugins[pl->slot] = pl;
        }
    }

  /* init plugins */
  Plugin * pl;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      pl = ch->plugins[i];
      if (!pl)
        continue;

      pl->track_pos = ch->track->pos;
      pl->track = ch->track;
      plugin_init_loaded (pl);
    }
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
 * Adds the automation track to both the Channel
 * and the AutomationTracklist of the Track.
 */
static inline void
channel_add_at (
  Channel * self,
  AutomationTrack * at)
{
  array_double_size_if_full (
    self->ats,
    self->num_ats,
    self->ats_size,
    AutomationTrack *);
  array_append (
    self->ats,
    self->num_ats,
    at);
  automation_tracklist_add_at (
    &self->track->automation_tracklist, at);
}

/**
 * Generates automation tracks for the channel.
 *
 * Should be called as soon as the track is
 * created.
 */
void
channel_generate_automation_tracks (
  Channel * channel)
{
  g_message (
    "Generating automation tracks for channel %s",
    channel->track->name);

  /*AutomationTracklist * atl =*/
    /*&channel->track->automation_tracklist;*/

  /* generate channel automatables if necessary */
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_FADER))
    {
      AutomationTrack * at =
        automation_track_new (
          automatable_create_fader (channel));
      channel_add_at (
        channel, at);
      at->created = 1;
      at->visible = 1;
    }
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_PAN))
    {
      AutomationTrack * at =
        automation_track_new (
          automatable_create_pan (channel));
      channel_add_at (
        channel, at);
    }
  if (!channel_get_automatable (
        channel,
        AUTOMATABLE_TYPE_CHANNEL_MUTE))
    {
      AutomationTrack * at =
        automation_track_new (
          automatable_create_mute (channel));
      channel_add_at (
        channel, at);
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
  /*if (AUDIO_ENGINE->midi_backend !=*/
        /*MIDI_BACKEND_DUMMY)*/
    /*{*/
      /*port_connect (AUDIO_ENGINE->midi_in,*/
                    /*ch->midi_in);*/
    /*}*/

  /* set default output */
  if (ch->track->type == TRACK_TYPE_MASTER)
    {
      ch->output = NULL;
      ch->output_pos = -1;
      port_connect (
        ch->stereo_out->l,
        AUDIO_ENGINE->stereo_out->l, 1);
      port_connect (
        ch->stereo_out->r,
        AUDIO_ENGINE->stereo_out->r, 1);
    }
  else
    {
      ch->output = P_MASTER_TRACK;
      ch->output_pos = P_MASTER_TRACK->pos;
    }

  if (ch->track->out_signal_type ==
        TYPE_AUDIO)
    {
      /* connect stereo in to stereo out through
       * fader */
      port_connect (
        ch->prefader.stereo_out->l,
        ch->fader.stereo_in->l, 1);
      port_connect (
        ch->prefader.stereo_out->r,
        ch->fader.stereo_in->r, 1);
      port_connect (
        ch->fader.stereo_out->l,
        ch->stereo_out->l, 1);
      port_connect (
        ch->fader.stereo_out->r,
        ch->stereo_out->r, 1);
    }
  else if (ch->track->out_signal_type ==
             TYPE_EVENT)
    {
      port_connect (
        ch->prefader.midi_out,
        ch->fader.midi_in, 1);
      port_connect (
        ch->fader.midi_out,
        ch->midi_out, 1);
    }

  /** Connect MIDI in and piano roll to MIDI out. */
  connect_ch_input_to_fader (ch);

  if (ch->track->type != TRACK_TYPE_MASTER)
    {
      if (ch->track->out_signal_type ==
            TYPE_AUDIO)
        {
          /* connect channel out ports to master */
          port_connect (
            ch->stereo_out->l,
            P_MASTER_TRACK->channel->stereo_in->l,
            1);
          port_connect (
            ch->stereo_out->r,
            P_MASTER_TRACK->channel->stereo_in->r,
            1);
        }
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
    output->channel->stereo_in->l, 1);
  port_connect (
    ch->stereo_out->r,
    output->channel->stereo_in->r, 1);

  ch->output = output;
  g_message ("setting output %s",
             output->name);

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_CHANNEL_OUTPUT_CHANGED,
               ch);
}

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 *
 * The array must be large enough.
 */
void
channel_append_all_ports (
  Channel * ch,
  Port ** ports,
  int *   size,
  int     include_plugins)
{
  int j, k;
  Track * tr = ch->track;
  PortType in_type =
    ch->track->in_signal_type;
  PortType out_type =
    ch->track->out_signal_type;

#define _ADD(port) \
  array_append ( \
    ports, (*size), \
    port)

#define ADD_PLUGIN_PORTS \
  if (!pl) \
    continue; \
\
  for (k = 0; k < pl->num_in_ports; k++) \
    _ADD (pl->in_ports[k]); \
  for (k = 0; k < pl->num_out_ports; k++) \
    _ADD (pl->out_ports[k])

  /* add channel ports */
  if (in_type == TYPE_AUDIO)
    {
      _ADD (ch->stereo_in->l);
      _ADD (ch->stereo_in->r);
    }
  else if (in_type == TYPE_EVENT)
    {
      _ADD (ch->midi_in);
      if (track_has_piano_roll (ch->track))
        {
          _ADD (ch->piano_roll);
        }
    }

  if (out_type == TYPE_AUDIO)
    {
      _ADD (ch->stereo_out->l);
      _ADD (ch->stereo_out->r);

      /* add fader ports */
      _ADD (ch->fader.stereo_in->l);
      _ADD (ch->fader.stereo_in->r);
      _ADD (ch->fader.stereo_out->l);
      _ADD (ch->fader.stereo_out->r);

      /* add prefader ports */
      _ADD (ch->prefader.stereo_in->l);
      _ADD (ch->prefader.stereo_in->r);
      _ADD (ch->prefader.stereo_out->l);
      _ADD (ch->prefader.stereo_out->r);
    }
  else if (out_type == TYPE_EVENT)
    {
      _ADD (ch->midi_out);

      /* add fader ports */
      _ADD (ch->fader.midi_in);
      _ADD (ch->fader.midi_out);

      /* add prefader ports */
      _ADD (ch->prefader.midi_in);
      _ADD (ch->prefader.midi_out);
    }

  Plugin * pl;
  if (include_plugins)
    {
      /* add plugin ports */
      for (j = 0; j < STRIP_SIZE; j++)
        {
          pl = tr->channel->plugins[j];

          ADD_PLUGIN_PORTS;
        }
    }

  for (j = 0; j < tr->num_modulators; j++)
    {
      pl = tr->modulators[j]->plugin;

      ADD_PLUGIN_PORTS;
    }
#undef ADD_PLUGIN_PORTS
#undef _ADD
}

/**
 * Prepares the Channel for serialization.
 */
void
channel_prepare_for_serialization (
  Channel * ch)
{
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->plugins[i])
        {
          array_append (
            ch->aggregated_plugins,
            ch->num_aggregated_plugins,
            ch->plugins[i]);
        }
    }
}

/**
 * Creates a channel of the given type with the
 * given label.
 */
Channel *
channel_new (
  Track *     track)
{
  g_return_val_if_fail (track, NULL);

  Channel * self = calloc (1, sizeof (Channel));

  self->track = track;

  self->ats_size = 12;
  self->ats =
    calloc (self->ats_size,
            sizeof (AutomationTrack *));

  /* create ports */
  self->stereo_in =
    stereo_ports_new_generic (
      1, _("Stereo in"),
      PORT_OWNER_TYPE_TRACK, track);
  self->stereo_out =
    stereo_ports_new_generic (
      0, _("Stereo out"),
      PORT_OWNER_TYPE_TRACK, track);

  char * pll =
    g_strdup (_("MIDI in"));
  self->midi_in =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      pll);
  self->midi_in->midi_events =
    midi_events_new (
      self->midi_in);
  g_free (pll);
  pll =
    g_strdup (_("MIDI out"));
  self->midi_out =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_OUTPUT,
      pll);
  self->midi_out->midi_events =
    midi_events_new (
      self->midi_out);
  g_free (pll);
  port_set_owner_track (
    self->midi_in,
    track);
  port_set_owner_track (
    self->midi_out,
    track);

  /* init plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      self->plugins[i] = NULL;
    }

  FaderType fader_type =
    track_get_fader_type (track);
  PassthroughProcessorType prefader_type =
    track_get_passthrough_processor_type (track);
  fader_init (
    &self->fader,
    fader_type,
    self);
  passthrough_processor_init (
    &self->prefader,
    prefader_type,
    self);

  /* set up piano roll port */
  char * tmp =
    g_strdup (_("Piano Roll"));
  self->piano_roll =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      tmp);
  self->piano_roll->identifier.flags =
    PORT_FLAG_PIANO_ROLL;
  self->piano_roll->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;
  self->piano_roll->identifier.track_pos =
    self->track->pos;
  self->piano_roll->track =
    self->track;
  self->piano_roll->midi_events =
    midi_events_new (
      self->piano_roll);

  return self;
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

void
channel_set_pan (void * _channel, float pan)
{
  Channel * channel = (Channel *) _channel;
  channel->fader.pan = pan;
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

  /* unexpose all JACK ports */
  Port * port;
  for (i = 0; i < pl->num_in_ports; i++)
    {
      port = pl->in_ports[i];

      if (port->internal_type ==
            INTERNAL_JACK_PORT)
        port_set_expose_to_jack (port, 0);
    }
  for (i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];

      if (port->internal_type ==
            INTERNAL_JACK_PORT)
        port_set_expose_to_jack (port, 0);
    }
}

/**
 * Removes a plugin at pos from the channel.
 *
 * If deleting_channel is 1, the automation tracks
 * associated with the plugin are not deleted at
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

  if (!deleting_plugin)
    {
      plugin_remove_ats_from_automation_tracklist (
        plugin);
    }

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

  channel->track->active = prev_active;

  if (gen_automatables)
    plugin_generate_automation_tracks (plugin);

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
  /*Plugin * plugin =*/
    /*channel_get_first_plugin (channel);*/

  /*if (plugin)*/
    /*{*/
      /*if (channel->type == CT_MIDI)*/
        /*{*/
          /* Connect/Disconnect MIDI editor manual
           * press port to the plugin */
          /*for (int i = 0; i < plugin->num_in_ports; i++)*/
            /*{*/
              /*Port * port = plugin->in_ports[i];*/
              /*if (port->identifier.type == TYPE_EVENT &&*/
                  /*port->identifier.flow == FLOW_INPUT)*/
                /*{*/
                  /*if (connect)*/
                    /*{*/
                      /*port_connect (AUDIO_ENGINE->midi_editor_manual_press, port, 1);*/
                    /*}*/
                  /*else*/
                    /*{*/
                    /*port_disconnect (AUDIO_ENGINE->midi_editor_manual_press, port);*/
                    /*}*/
                /*}*/
            /*}*/
        /*}*/
    /*}*/
  if (connect)
    port_connect (
      AUDIO_ENGINE->midi_editor_manual_press,
      channel->midi_in, 1);
  else
    port_disconnect (
      AUDIO_ENGINE->midi_editor_manual_press,
      channel->midi_in);

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
 * Convenience function to get an automatable
 * of the given type for the channel.
 */
Automatable *
channel_get_automatable (
  Channel *       channel,
  AutomatableType type)
{
  AutomationTrack * at;
  Automatable * a;
  for (int i = 0;
       i < channel->track->
         automation_tracklist.num_ats; i++)
    {
      at =
        channel->track->automation_tracklist.ats[i];
      a = at->automatable;

      if (type == a->type)
        return a;
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
    channel_new (track);

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

  clone->fader.channel = clone;
  clone->prefader.channel = clone;
  fader_copy (&ch->fader, &clone->fader);

#undef COPY_MEMBER

  /* TODO clone port connections, same for
   * plugins */

  return clone;
}

/**
 * Removes the AutomationTrack's associated with
 * this channel from the AutomationTracklist in the
 * corresponding Track.
 */
void
channel_remove_ats_from_automation_tracklist (
  Channel * ch)
{
  for (int i = 0; i < ch->num_ats; i++)
    {
      automation_tracklist_remove_at (
        &ch->track->automation_tracklist,
        ch->ats[i], F_NO_FREE);
    }
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
  port_disconnect_all (channel->midi_out);
  port_disconnect_all (channel->piano_roll);
  port_disconnect_all (channel->stereo_out->l);
  port_disconnect_all (channel->stereo_out->r);

  passthrough_processor_disconnect_all (
    &channel->prefader);
  fader_disconnect_all (&channel->fader);

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
  port_free (channel->midi_out);
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

  /* remove automation tracks - they are already
   * free'd in track_free */
  channel_remove_ats_from_automation_tracklist (
    channel);
  free (channel->ats);

  if (channel->widget)
    gtk_widget_destroy (
      GTK_WIDGET (channel->widget));
}

