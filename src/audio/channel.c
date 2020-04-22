/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex and zrythm dot org>
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
#include "audio/engine_rtmidi.h"
#include "audio/ext_port.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/modulator.h"
#include "audio/pan.h"
#include "audio/rtmidi_device.h"
#include "audio/track.h"
#include "audio/track_processor.h"
#include "audio/transport.h"
#include "audio/windows_mme_device.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_gtk.h"
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
 * Connect ports in the case of !prev && !next.
 */
static void
connect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  Track * track = channel_get_track (ch);

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel
   * stereo out. disconnect it */
  track_processor_disconnect_from_prefader (
    &track->processor);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (
    &track->processor, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin to stereo out */
  plugin_connect_to_prefader (pl, ch);
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
  Track * track = channel_get_track (ch);

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin.
   * disconnect it */
  track_processor_disconnect_from_plugin (
    &track->processor, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (
    &track->processor, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin's audio outs to next
   * plugin */
  plugin_connect_to_plugin (pl, next_pl);
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
  plugin_disconnect_from_prefader (
    prev_pl, ch);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's outs to
   * plugin */
  plugin_connect_to_plugin (
    prev_pl, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin output ports to stereo_out */
  plugin_connect_to_prefader (
    pl, ch);
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
  plugin_disconnect_from_plugin (
    prev_pl, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to
   * plugin */
  plugin_connect_to_plugin (
    prev_pl, pl);

  /* ------------------------------------------
   * Connect output ports
   * ------------------------------------------ */

  /* connect plugin's audio outs to next
   * plugin */
  plugin_connect_to_plugin (
    pl, next_pl);
}

/**
 * Disconnect ports in the case of !prev && !next.
 */
static void
disconnect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  Track * track = channel_get_track (ch);

  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_processor_disconnect_from_plugin (
    &track->processor, pl);

  /* --------------------------------------
   * disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin from stereo out */
  plugin_disconnect_from_prefader (
    pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to
   * channel stereo out. connect it */
  track_processor_connect_to_prefader (
    &track->processor);
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
  Track * track = channel_get_track (ch);

  /* -------------------------------------------
   * Disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_processor_disconnect_from_plugin (
    &track->processor, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin's midi & audio outs from next
   * plugin */
  plugin_disconnect_from_plugin (
    pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to next
   * plugin. connect it */
  track_processor_connect_to_plugin (
    &track->processor, next_pl);
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
  plugin_disconnect_from_plugin (
    prev_pl, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin output ports from stereo
   * out */
  plugin_disconnect_from_prefader (
    pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to channel
   * stereo out. connect it */
  plugin_connect_to_prefader (
    prev_pl, ch);
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
  plugin_disconnect_from_plugin (
    prev_pl, pl);

  /* ------------------------------------------
   * Disconnect output ports
   * ------------------------------------------ */

  /* disconnect plugin's audio outs from next
   * plugin */
  plugin_disconnect_from_plugin (
    pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to the next pl.
   * connect them */
  plugin_connect_to_plugin (
    prev_pl, next_pl);
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
  int j;
  Track * tr = channel_get_track (self);
  PortType out_type = tr->out_signal_type;

  /* clear buffers */
  track_processor_clear_buffers (
    &tr->processor);
  passthrough_processor_clear_buffers (
    &self->prefader);
  fader_clear_buffers (&self->fader);

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
      plugin = self->inserts[j];
      if (plugin)
        plugin_prepare_process (plugin);
      plugin = self->midi_fx[j];
      if (plugin)
        plugin_prepare_process (plugin);
    }
  if (self->instrument)
    plugin_prepare_process (self->instrument);

  if (tr->in_signal_type == TYPE_EVENT)
    {
#ifdef HAVE_RTMIDI
      /* extract the midi events from the ring
       * buffer */
      if (AUDIO_ENGINE->midi_backend ==
            MIDI_BACKEND_RTMIDI)
        {
          port_prepare_rtmidi_events (
            tr->processor.midi_in);
        }
#endif

      /* copy the cached MIDI events to the
       * MIDI events in the MIDI in port */
      midi_events_dequeue (
        tr->processor.midi_in->midi_events);
    }
}

void
channel_init_loaded (Channel * ch)
{
  g_message ("initing channel");
  Track * track = channel_get_track (ch);
  g_warn_if_fail (track);

  ch->magic = CHANNEL_MAGIC;

  /* fader */
  ch->prefader.track_pos = track->pos;
  ch->fader.track_pos = track->pos;

  passthrough_processor_init_loaded (
    &ch->prefader);
  fader_init_loaded (&ch->fader);

  PortType out_type =
    track->out_signal_type;

  switch (out_type)
    {
    case TYPE_EVENT:
      ch->midi_out->midi_events =
        midi_events_new (
          ch->midi_out);
      ch->midi_out->id.owner_type =
        PORT_OWNER_TYPE_TRACK;
      break;
    case TYPE_AUDIO:
      break;
    default:
      break;
    }

  /* init plugins */
  Plugin * pl;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      pl = ch->inserts[i];
      if (pl)
        {
          pl->id.track_pos = ch->track_pos;
          pl->id.slot = i;
          pl->id.slot_type = PLUGIN_SLOT_INSERT;
          plugin_init_loaded (pl);
        }
      pl = ch->midi_fx[i];
      if (pl)
        {
          pl->id.track_pos = ch->track_pos;
          pl->id.slot = i;
          pl->id.slot_type = PLUGIN_SLOT_MIDI_FX;
          plugin_init_loaded (pl);
        }
    }
  if (ch->instrument)
    {
      pl = ch->instrument;
      pl->id.track_pos = ch->track_pos;
      pl->id.slot = 0;
      pl->id.slot_type = PLUGIN_SLOT_INSTRUMENT;
      plugin_init_loaded (pl);
    }
}

/**
 * Exposes the channel's ports to the backend.
 */
void
channel_expose_ports_to_backend (
  Channel * ch)
{
  Track * tr =
    channel_get_track (ch);

  if (tr->in_signal_type ==
        TYPE_AUDIO)
    {
      port_set_expose_to_backend (
        tr->processor.stereo_in->l , 1);
      port_set_expose_to_backend (
        tr->processor.stereo_in->r , 1);
    }
  if (tr->in_signal_type ==
        TYPE_EVENT)
    {
      port_set_expose_to_backend (
        tr->processor.midi_in , 1);
    }
  if (tr->out_signal_type ==
        TYPE_AUDIO)
    {
      port_set_expose_to_backend (
        ch->stereo_out->l , 1);
      port_set_expose_to_backend (
        ch->stereo_out->r , 1);
    }
  if (tr->out_signal_type ==
        TYPE_EVENT)
    {
      port_set_expose_to_backend (
        ch->midi_out , 1);
    }
}

#ifdef _WOE32
/**
 * Reconnects the given Channel's midi in.
 */
static void
reconnect_windows_mme_ext_in (
  Channel * ch,
  Port *    in_port)
{
  zix_sem_wait (&in_port->mme_connections_sem);

  /* disconnect current connections */
  in_port->num_mme_connections = 0;

  /* connect to all external midi ins */
  if (ch->all_midi_ins)
    {
      for (int i = 0;
           i < AUDIO_ENGINE->num_mme_in_devs; i++)
        {
          in_port->mme_connections[
            in_port->num_mme_connections++] =
              AUDIO_ENGINE->mme_in_devs[i];
        }
    }
  /* connect to selected ports */
  else
    {
      int num = ch->num_ext_midi_ins;
      ExtPort ** arr = ch->ext_midi_ins;

      for (int i = 0; i < num; i++)
        {
          if (!arr[i]->mme_dev)
            {
              g_warn_if_reached ();
              continue;
            }
          in_port->mme_connections[
            in_port->num_mme_connections++] =
              arr[i]->mme_dev;
        }
    }

  zix_sem_post (&in_port->mme_connections_sem);
}
#endif

#ifdef HAVE_JACK
/**
 * Reconnects the given port (either
 * TrackProcessor's stereo in L/R or midi in).
 */
static void
reconnect_jack_ext_in (
  Channel * ch,
  Port *    in_port)
{
  int i = 0;
  int ret;
  Track * track =
    channel_get_track (ch);
  TrackProcessor * processor =
    &track->processor;
  const int is_midi_in =
    in_port == processor->midi_in;
  const int is_stereo_l_in =
    processor->stereo_in &&
    in_port == processor->stereo_in->l;
  const int is_stereo_r_in =
    processor->stereo_in &&
    in_port == processor->stereo_in->r;

  /* disconnect */
  const char ** prev_ports =
    jack_port_get_all_connections (
      AUDIO_ENGINE->client,
      (jack_port_t *)
        in_port->data);
  if (prev_ports)
    {
      i = 0;
      while (prev_ports[i] != NULL)
        {
          ret =
            jack_disconnect (
              AUDIO_ENGINE->client,
              prev_ports[i],
              jack_port_name (
                (jack_port_t *)
                in_port->data));
          g_warn_if_fail (!ret);
          i++;
        }
    }

  /* connect to all external midi ins */
  if ((is_midi_in &&
       ch->all_midi_ins) ||
      (is_stereo_l_in &&
       ch->all_stereo_l_ins) ||
      (is_stereo_r_in &&
       ch->all_stereo_r_ins))
    {
      const char ** ports =
        jack_get_ports (
          AUDIO_ENGINE->client,
          NULL,
          is_midi_in ?
            JACK_DEFAULT_MIDI_TYPE :
            JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsOutput |
          JackPortIsPhysical);

      if (ports)
        {
          i = 0;
          while (ports[i] != NULL)
            {
              ret =
                jack_connect (
                  AUDIO_ENGINE->client,
                  ports[i],
                  jack_port_name (
                    (jack_port_t *)
                    in_port->data));
              g_warn_if_fail (!ret);
              i++;
            }
          jack_free (ports);
        }
    }
  /* connect to selected ports */
  else
    {
      int num = 0;
      ExtPort ** arr = NULL;
      if (is_midi_in)
        {
          num = ch->num_ext_midi_ins;
          arr = ch->ext_midi_ins;
        }
      else if (is_stereo_l_in)
        {
          num = ch->num_ext_stereo_l_ins;
          arr = ch->ext_stereo_l_ins;
        }
      else if (is_stereo_r_in)
        {
          num = ch->num_ext_stereo_r_ins;
          arr = ch->ext_stereo_r_ins;
        }

      for (i = 0; i < num; i++)
        {
          ret =
            jack_connect (
              AUDIO_ENGINE->client,
              arr[i]->full_name,
              jack_port_name (
                (jack_port_t *)
                in_port->data));
          g_warn_if_fail (!ret);
        }
    }
}
#endif

#ifdef HAVE_RTMIDI
/**
 * Reconnects the given port (either
 * TrackProcessor's stereo in L/R or midi in).
 */
static void
reconnect_rtmidi_ext_in (
  Channel * ch,
  Port *    in_port)
{
  int i = 0;
  Track * track =
    channel_get_track (ch);
  TrackProcessor * processor =
    &track->processor;
  const int is_midi_in =
    in_port == processor->midi_in;
  if (!is_midi_in)
    return;

  char lbl[600];
  port_get_full_designation (in_port, lbl);

  /* disconnect */
  for (i = 0; i < in_port->num_rtmidi_ins; i++)
    {
      rtmidi_device_close (
        in_port->rtmidi_ins[i], 1);
    }
  in_port->num_rtmidi_ins = 0;

  /* connect to all external midi ins */
  if (ch->all_midi_ins)
    {
      unsigned int num_ports =
        engine_rtmidi_get_num_in_ports (
          AUDIO_ENGINE);

      for (unsigned int j = 0; j < num_ports; j++)
        {
          RtMidiDevice * dev =
            rtmidi_device_new (1, j, in_port);
          in_port->rtmidi_ins[
            in_port->num_rtmidi_ins++] =
              dev;
          rtmidi_device_open (dev, 1);
        }
    }
  /* connect to selected ports */
  else
    {
      int num = 0;
      ExtPort ** arr = NULL;
      num = ch->num_ext_midi_ins;
      arr = ch->ext_midi_ins;

      for (i = 0; i < num; i++)
        {
          g_message ("RTMIDI id %d",
            arr[i]->rtmidi_id);
          RtMidiDevice * dev =
            rtmidi_device_new (
              1, arr[i]->rtmidi_id, in_port);
          in_port->rtmidi_ins[
            in_port->num_rtmidi_ins++] =
              dev;
          rtmidi_device_open (dev, 1);
        }
    }
}
#endif

/**
 * Called when the input has changed for Midi,
 * Instrument or Audio tracks.
 */
void
channel_reconnect_ext_input_ports (
  Channel * ch)
{
  Track * track =
    channel_get_track (ch);
  if (track->type == TRACK_TYPE_INSTRUMENT ||
      track->type == TRACK_TYPE_MIDI)
    {
      Port * midi_in =
        track->processor.midi_in;

      /* if the project was loaded with another
       * backend, the port might not be exposed
       * yet, so expose it */
      port_set_expose_to_backend (midi_in, 1);

      switch (AUDIO_ENGINE->midi_backend)
        {
#ifdef HAVE_JACK
        case MIDI_BACKEND_JACK:
          reconnect_jack_ext_in (ch, midi_in);
          break;
#endif
#ifdef _WOE32
        case MIDI_BACKEND_WINDOWS_MME:
          reconnect_windows_mme_ext_in (
            ch, midi_in);
          break;
#endif
#ifdef HAVE_RTMIDI
        case MIDI_BACKEND_RTMIDI:
          reconnect_rtmidi_ext_in (ch, midi_in);
          break;
#endif
        default:
          break;
        }
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      /* if the project was loaded with another
       * backend, the port might not be exposed
       * yet, so expose it */
      port_set_expose_to_backend (
        track->processor.stereo_in->l, 1);
      port_set_expose_to_backend (
        track->processor.stereo_in->r, 1);

      if (AUDIO_ENGINE->audio_backend ==
            AUDIO_BACKEND_JACK &&
          AUDIO_ENGINE->midi_backend ==
            MIDI_BACKEND_JACK)
        {
#ifdef HAVE_JACK
          reconnect_jack_ext_in (
            ch, track->processor.stereo_in->l);
          reconnect_jack_ext_in (
            ch, track->processor.stereo_in->r);
#endif
        }
    }
}

/**
 * Adds to (or subtracts from) the pan.
 */
void
channel_add_balance_control (
  void * _channel, float pan)
{
  Channel * channel = (Channel *) _channel;

  port_set_control_value (
    channel->fader.balance,
    CLAMP (
      channel->fader.balance->control + pan,
      0.f, 1.f),
    0, 0);
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
 * Generates automation tracks for the channel.
 *
 * Should be called as soon as the track is
 * created.
 */
void
channel_generate_automation_tracks (
  Channel * self)
{
  Track * track =
    channel_get_track (self);
  g_message (
    "Generating automation tracks for channel %s",
    track->name);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);

  /* fader */
  AutomationTrack * at =
    automation_track_new (self->fader.amp);
  automation_tracklist_add_at (atl, at);
  at->created = 1;
  at->visible = 1;

  /* balance */
  at =
    automation_track_new (self->fader.balance);
  automation_tracklist_add_at (atl, at);

  /* mute */
  at = automation_track_new (track->mute);
  automation_tracklist_add_at (atl, at);
}

/**
 * Connects the channel's ports.
 */
void
channel_connect (
  Channel * ch)
{
  Track * tr = channel_get_track (ch);

  /* set default output */
  if (tr->type == TRACK_TYPE_MASTER)
    {
      ch->output_pos = -1;
      ch->has_output = 0;
      port_connect (
        ch->stereo_out->l,
        MONITOR_FADER->stereo_in->l,
        1);
      port_connect (
        ch->stereo_out->r,
        MONITOR_FADER->stereo_in->r,
        1);
    }
  else if (tr->out_signal_type == TYPE_AUDIO)
    {
      ch->output_pos = P_MASTER_TRACK->pos;
      ch->has_output = 1;
    }

  if (tr->out_signal_type ==
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
  else if (tr->out_signal_type ==
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
  track_processor_connect_to_prefader (
    &tr->processor);

  if (tr->type != TRACK_TYPE_MASTER)
    {
      if (tr->out_signal_type == TYPE_AUDIO)
        {
          /* connect channel out ports to master */
          port_connect (
            ch->stereo_out->l,
            P_MASTER_TRACK->processor.
              stereo_in->l,
            1);
          port_connect (
            ch->stereo_out->r,
            P_MASTER_TRACK->processor.
              stereo_in->r,
            1);
        }
    }

  /* expose ports to backend */
  channel_expose_ports_to_backend (ch);

  /* connect the designated midi inputs */
  channel_reconnect_ext_input_ports (ch);
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
  if (ch->has_output)
    {
      Track * track =
        channel_get_output_track (ch);
      /* disconnect Channel's output from the
       * current
       * output channel */
      switch (track->in_signal_type)
        {
        case TYPE_AUDIO:
          port_disconnect (
            ch->stereo_out->l,
            track->processor.stereo_in->l);
          port_disconnect (
            ch->stereo_out->r,
            track->processor.stereo_in->r);
          break;
        case TYPE_EVENT:
          port_disconnect (
            ch->midi_out,
            track->processor.midi_in);
          break;
        default:
          break;
        }
    }

  /* connect Channel's output to the given output
   */
  switch (output->in_signal_type)
    {
    case TYPE_AUDIO:
      port_connect (
        ch->stereo_out->l,
        output->processor.stereo_in->l, 1);
      port_connect (
        ch->stereo_out->r,
        output->processor.stereo_in->r, 1);
      break;
    case TYPE_EVENT:
      port_connect (
        ch->midi_out,
        output->processor.midi_in, 1);
      break;
    default:
      break;
    }

  if (output)
    {
      ch->has_output = 1;
      ch->output_pos = output->pos;
    }
  else
    ch->has_output = 0;

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_CHANNEL_OUTPUT_CHANGED, ch);
}

Track *
channel_get_track (
  Channel * self)
{
  if (self->track)
    return self->track;
  else
    {
      g_return_val_if_fail (
        self &&
          self->track_pos < TRACKLIST->num_tracks,
        NULL);
      Track * track =
        TRACKLIST->tracks[self->track_pos];
      g_return_val_if_fail (track, NULL);
      self->track = track;

      return track;
    }
}

Track *
channel_get_output_track (
  Channel * self)
{
  g_return_val_if_fail (
    self &&
    self->output_pos < TRACKLIST->num_tracks,
    NULL);
  Track * track =
    TRACKLIST->tracks[self->output_pos];
  g_return_val_if_fail (track, NULL);

  return track;
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
  Track * tr = channel_get_track (ch);
  g_warn_if_fail (tr);
  PortType in_type =
    tr->in_signal_type;
  PortType out_type =
    tr->out_signal_type;

#define _ADD(port) \
  array_append ( \
    ports, (*size), \
    port)

#define ADD_PLUGIN_PORTS \
  if (!pl) \
    continue; \
\
  for (k = 0; k < pl->num_in_ports; k++) \
    { \
      g_warn_if_fail (pl->in_ports[k]); \
      _ADD (pl->in_ports[k]); \
    } \
  for (k = 0; k < pl->num_out_ports; k++) \
    { \
      g_warn_if_fail (pl->out_ports[k]); \
      _ADD (pl->out_ports[k]); \
    }

  /* add channel ports */
  if (in_type == TYPE_AUDIO)
    {
      g_warn_if_fail (tr->processor.stereo_in->l);
      _ADD (tr->processor.stereo_in->l);
      _ADD (tr->processor.stereo_in->r);
      g_warn_if_fail (tr->processor.stereo_out->l);
      _ADD (tr->processor.stereo_out->l);
      _ADD (tr->processor.stereo_out->r);
    }
  else if (in_type == TYPE_EVENT)
    {
      g_warn_if_fail (tr->processor.midi_in);
      g_warn_if_fail (tr->processor.midi_out);
      _ADD (tr->processor.midi_in);
      _ADD (tr->processor.midi_out);
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

  /* add fader amp and balance control */
  g_return_if_fail (
    ch->fader.amp && ch->fader.balance);
  _ADD (ch->fader.amp);
  _ADD (ch->fader.balance);
  _ADD (ch->track->mute);

  Plugin * pl;
  if (include_plugins)
    {
      /* add plugin ports */
      for (j = 0; j < STRIP_SIZE; j++)
        {
          pl = ch->inserts[j];
          ADD_PLUGIN_PORTS;

          pl = ch->midi_fx[j];
          ADD_PLUGIN_PORTS;
        }

      if (ch->instrument)
        {
          for (j = 0; j < 1; j++)
            {
              pl = ch->instrument;
              ADD_PLUGIN_PORTS;
            }
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
 * Inits the stereo ports of the Channel while
 * exposing them to the backend.
 *
 * This assumes the caller already checked that
 * this channel should have the given ports
 * enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if
 *   new.
 */
static void
init_stereo_out_ports (
  Channel * self,
  int       loading)
{
  char str[80];
  strcpy (str, "Stereo out");
  Port * l, * r;
  StereoPorts ** sp =
    &self->stereo_out;
  PortFlow flow = FLOW_OUTPUT;

  if (loading)
    {
      l = NULL;
      r = NULL;
    }
  else
    {
      strcat (str, " L");
      l = port_new_with_type (
        TYPE_AUDIO, flow, str);

      str[10] = '\0';
      strcat (str, " R");
      r = port_new_with_type (
        TYPE_AUDIO,
        flow,
        str);
    }

  port_set_owner_track_from_channel (l, self);
  port_set_owner_track_from_channel (r, self);

  *sp =
    stereo_ports_new_from_existing (l, r);
}

/**
 * Inits the MIDI In port of the Channel while
 * exposing it to JACK.
 *
 * This assumes the caller already checked that
 * this channel should have the given MIDI port
 * enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if
 *   new.
 */
static void
init_midi_port (
  Channel * self,
  int       loading)
{
  const char * str = "MIDI out";
  Port ** port =
    &self->midi_out;
  PortFlow flow = FLOW_OUTPUT;

  *port =
    port_new_with_type (
      TYPE_EVENT, flow, str);

  Track * track = channel_get_track (self);
  port_set_owner_track (*port, track);
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

  self->magic = CHANNEL_MAGIC;
  self->track_pos = track->pos;
  self->track = track;

  /* autoconnect to all midi ins and midi chans */
  self->all_midi_ins = 1;
  self->all_midi_channels = 1;

  /* create ports */
  switch (track->out_signal_type)
    {
    case TYPE_AUDIO:
      init_stereo_out_ports (
        self, 0);
      break;
    case TYPE_EVENT:
      init_midi_port (
        self, 0);
      break;
    default:
      break;
    }

  FaderType fader_type =
    track_get_fader_type (track);
  PassthroughProcessorType prefader_type =
    track_get_passthrough_processor_type (track);
  fader_init (
    &self->fader, fader_type, self);
  passthrough_processor_init (
    &self->prefader, prefader_type, self);
  track_processor_init (&track->processor, track);

  /* init sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_init (
        &self->sends[i], self->track_pos, i);
    }

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
channel_set_balance_control (
  void * _channel,
  float pan)
{
  Channel * channel = (Channel *) _channel;
  port_set_control_value (
    channel->fader.balance, pan, 0, 0);
}

float
channel_get_balance_control (
  void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return
    port_get_control_value (
      channel->fader.balance, 0);
}

static float
get_current_rms (
  Channel * channel,
  int       left)
{
  float rms = 0.f;
  Track * track = channel_get_track (channel);
  if (track->out_signal_type == TYPE_EVENT)
    {
      ChannelWidget * cw = channel->widget;
      Port * port = channel->midi_out;
      int has_midi_events = 0;
      if (port->write_ring_buffers)
        {
          MidiEvent event;
          while (
            zix_ring_peek (
              port->midi_ring, &event,
              sizeof (MidiEvent)) > 0)
            {
              if (event.systime >
                    cw->last_midi_trigger_time)
                {
                  has_midi_events = 1;
                  cw->last_midi_trigger_time =
                    event.systime;
                  break;
                }
            }
        }
      else
        {
          has_midi_events =
            g_atomic_int_compare_and_exchange (
              &port->has_midi_events, 1, 0);
            if (has_midi_events)
              {
                cw->last_midi_trigger_time =
                  g_get_monotonic_time ();
              }
        }

      if (has_midi_events)
        {
          return 1.f;
        }
      else
        {
          /** 350 ms */
          static const float MAX_TIME = 350000.f;
          gint64 time_diff =
            g_get_monotonic_time () -
            cw->last_midi_trigger_time;
          if ((float) time_diff < MAX_TIME)
            {
              return
                1.f - (float) time_diff / MAX_TIME;
            }
          else
            {
              return 0.f;
            }
        }
    }
  else if (track->out_signal_type == TYPE_AUDIO)
    {
      rms =
        port_get_rms_db (
          left ?
            channel->stereo_out->l :
            channel->stereo_out->r,
          1);
    }
  return rms;
}

float
channel_get_current_l_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  float rms = get_current_rms (channel, 1);
  return rms;
}

float
channel_get_current_r_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  float rms = get_current_rms (channel, 0);
  return rms;
}

float
channel_get_current_l_peak (void * _channel)
{
  Channel * ch= (Channel *) _channel;
  gint64 now = g_get_monotonic_time ();
  if (now - ch->stereo_out->l->peak_timestamp <
        600000)
    {
      return ch->stereo_out->l->peak;
    }
  return -1.f;
}

float
channel_get_current_r_peak (void * _channel)
{
  Channel * ch= (Channel *) _channel;
  gint64 now = g_get_monotonic_time ();
  if (now - ch->stereo_out->r->peak_timestamp <
        600000)
    {
      return ch->stereo_out->r->peak;
    }
  return -1.f;
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
  PluginSlotType slot_type = pl->id.slot_type;
  Plugin ** prev_plugins = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      prev_plugins = ch->inserts;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      prev_plugins = ch->midi_fx;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      prev_plugins = ch->midi_fx;
      break;
    default:
      g_return_if_reached ();
    }
  Plugin ** next_plugins = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      next_plugins = ch->inserts;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      next_plugins = ch->midi_fx;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      next_plugins = ch->inserts;
      break;
    default:
      g_return_if_reached ();
    }

  Plugin * next_plugin = NULL;
  for (i = pos + 1; i < STRIP_SIZE; i++)
    {
      next_plugin = next_plugins[i];
      if (next_plugin)
        break;
    }
  if (!next_plugin &&
      slot_type == PLUGIN_SLOT_MIDI_FX)
    {
      if (ch->instrument)
        next_plugin = ch->instrument;
      else
        {
          for (i = 0; i < STRIP_SIZE; i++)
            {
              next_plugin = ch->inserts[i];
              if (next_plugin)
                break;
            }
        }
    }

  Plugin * prev_plugin = NULL;
  for (i = pos - 1; i >= 0; i--)
    {
      prev_plugin = prev_plugins[i];
      if (prev_plugin)
        break;
    }
  if (!prev_plugin &&
      slot_type == PLUGIN_SLOT_INSERT)
    {
      if (ch->instrument)
        prev_plugin = ch->instrument;
      else
        {
          for (i = STRIP_SIZE - 1; i >= 0; i--)
            {
              prev_plugin = ch->midi_fx[i];
              if (prev_plugin)
                break;
            }
        }
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

      if (port->internal_type !=
            INTERNAL_NONE)
        port_set_expose_to_backend (port, 0);
    }
  for (i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];

      if (port->internal_type !=
            INTERNAL_NONE)
        port_set_expose_to_backend (port, 0);
    }
}

/**
 * Removes a plugin at pos from the channel.
 *
 * If deleting_channel is true, the automation tracks
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
  Channel *      channel,
  PluginSlotType slot_type,
  int            slot,
  bool           deleting_plugin,
  bool           deleting_channel,
  bool           recalc_graph)
{
  Plugin * plugin = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      plugin = channel->inserts[slot];
      break;
    case PLUGIN_SLOT_MIDI_FX:
      plugin = channel->midi_fx[slot];
      break;
    default:
      break;
    }
  g_return_if_fail (plugin);
  g_warn_if_fail (
    plugin->id.track_pos == channel->track_pos);

  plugin_remove_ats_from_automation_tracklist (
    plugin, deleting_plugin);

  Track * track = channel_get_track (channel);
  g_message (
    "Removing %s from %s:%d",
    plugin->descr->name, track->name, slot);

  channel_disconnect_plugin_from_strip (
    channel, slot, plugin);

  /* if deleting plugin disconnect the plugin
   * entirely */
  if (deleting_plugin)
    {
      if (plugin_is_selected (plugin))
        {
          mixer_selections_remove_slot (
            MIXER_SELECTIONS, plugin->id.slot,
            slot_type,
            F_PUBLISH_EVENTS);
        }

      /* close the UI */
      plugin_close_ui (plugin);

      plugin_disconnect (plugin);
      free_later (plugin, plugin_free);
    }

  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      channel->inserts[slot] = NULL;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      channel->midi_fx[slot] = NULL;
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  /* if not deleting plugin (moving, etc.) just
   * disconnect its connections to the prev/
   * next slot or the channel if first/last */
  /*else*/
    /*{*/
      /*channel_disconnect_plugin_from_strip (*/
        /*channel, pos, plugin);*/
    /*}*/

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
 * @param slot The position in the strip starting
 *   from 0.
 * @param plugin The plugin to add.
 * @param confirm Confirm if an existing plugin
 *   will be overwritten.
 * @param gen_automatables Generatate plugin
 *   automatables.
 *   To be used when creating a new plugin only.
 * @param recalc_graph Recalculate mixer graph.
 * @param pub_events Publish events.
 *
 * @return 1 if plugin added, 0 if not.
 */
int
channel_add_plugin (
  Channel * self,
  PluginSlotType slot_type,
  int       slot,
  Plugin *  plugin,
  int       confirm,
  int       gen_automatables,
  int       recalc_graph,
  int       pub_events)
{
  int i;
  Track * track = channel_get_track (self);
  int prev_active = track->active;
  track->active = 0;

  Plugin ** plugins = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      plugins = self->inserts;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      plugins = self->midi_fx;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      break;
    default:
      g_return_val_if_reached (0);
    }

  Plugin * existing_pl = NULL;
  if (slot_type != PLUGIN_SLOT_INSTRUMENT)
    {
      /* confirm if another plugin exists */
      existing_pl = plugins[slot];
      if (confirm && existing_pl)
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
    }

  /* free current plugin */
  if (existing_pl)
    {
      channel_remove_plugin (
        self, slot_type, slot, 1, 0,
        F_NO_RECALC_GRAPH);
    }

  g_message (
    "Inserting %s %s at %s:%d",
    plugin_slot_type_to_string (slot_type),
    plugin->descr->name, track->name, slot);
  if (slot_type == PLUGIN_SLOT_INSTRUMENT)
    {
      self->instrument = plugin;
    }
  else
    {
      plugins[slot] = plugin;
    }
  plugin_set_channel_and_slot (
    plugin, self, slot_type, slot);

  Plugin ** prev_plugins = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      prev_plugins = self->inserts;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      prev_plugins = self->midi_fx;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      prev_plugins = self->midi_fx;
      break;
    default:
      g_return_val_if_reached (0);
    }
  Plugin ** next_plugins = NULL;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      next_plugins = self->inserts;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      next_plugins = self->midi_fx;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      next_plugins = self->inserts;
      break;
    default:
      g_return_val_if_reached (0);
    }

  Plugin * next_pl = NULL;
  for (i =
         (slot_type == PLUGIN_SLOT_INSTRUMENT ?
            0 : slot + 1); i < STRIP_SIZE; i++)
    {
      next_pl = next_plugins[i];
      if (next_pl)
        break;
    }
  if (!next_pl &&
      slot_type == PLUGIN_SLOT_MIDI_FX)
    {
      if (self->instrument)
        next_pl = self->instrument;
      else
        {
          for (i = 0; i < STRIP_SIZE; i++)
            {
              next_pl = self->inserts[i];
              if (next_pl)
                break;
            }
        }
    }

  Plugin * prev_pl = NULL;
  for (i =
         (slot_type == PLUGIN_SLOT_INSTRUMENT ?
            STRIP_SIZE - 1 : slot - 1); i >= 0; i--)
    {
      prev_pl = prev_plugins[i];
      if (prev_pl)
        break;
    }
  if (!prev_pl &&
      slot_type == PLUGIN_SLOT_INSERT)
    {
      if (self->instrument)
        prev_pl = self->instrument;
      else
        {
          for (i = STRIP_SIZE - 1; i >= 0; i--)
            {
              prev_pl = self->midi_fx[i];
              if (prev_pl)
                break;
            }
        }
    }

  /* ------------------------------------------
   * connect ports
   * ------------------------------------------ */

  if (!prev_pl && !next_pl)
    connect_no_prev_no_next (
      self, plugin);
  else if (!prev_pl && next_pl)
    connect_no_prev_next (
      self, plugin, next_pl);
  else if (prev_pl && !next_pl)
    connect_prev_no_next (
      self, prev_pl, plugin);
  else if (prev_pl && next_pl)
    connect_prev_next (
      self, prev_pl, plugin, next_pl);

  track->active = prev_active;

  if (gen_automatables)
    plugin_generate_automation_tracks (
      plugin, track);

  if (pub_events)
    {
      EVENTS_PUSH (ET_PLUGIN_ADDED, plugin);
    }

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  return 1;
}

/**
 * Updates the track position in the channel and
 * all related ports and identifiers.
 */
void
channel_update_track_pos (
  Channel * self,
  int       pos)
{
  self->track_pos = pos;

  Port * ports[80000];
  int    num_ports = 0;
  channel_append_all_ports (
    self, ports, &num_ports, 1);

  for (int i = 0; i < num_ports; i++)
    {
      g_warn_if_fail (ports[i]);
      port_update_track_pos (ports[i], pos);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * pl = self->inserts[i];
      if (pl)
        {
          plugin_set_track_pos (pl, pos);
        }
      pl = self->midi_fx[i];
      if (pl)
        {
          plugin_set_track_pos (pl, pos);
        }
    }
  if (self->instrument)
    {
      plugin_set_track_pos (self->instrument, pos);
    }

  if (self->midi_out)
    {
      port_update_track_pos (self->midi_out, pos);
    }
  if (self->stereo_out)
    {
      port_update_track_pos (
        self->stereo_out->l, pos);
      port_update_track_pos (
        self->stereo_out->r, pos);
    }

  fader_update_track_pos (&self->fader, pos);
  passthrough_processor_update_track_pos (
    &self->prefader, pos);
}

/**
 * Connects or disconnects the MIDI editor key press
 * port to the channel's input.
 *
 * @param connect Connect or not.
 */
void
channel_reattach_midi_editor_manual_press_port (
  Channel * channel,
  int       connect,
  const int recalc_graph)
{
  Track * track = channel_get_track (channel);
  if (connect)
    port_connect (
      AUDIO_ENGINE->midi_editor_manual_press,
      track->processor.midi_in, 1);
  else
    port_disconnect (
      AUDIO_ENGINE->midi_editor_manual_press,
      track->processor.midi_in);

  if (recalc_graph)
    mixer_recalc_graph (MIXER);
}

/**
 * Convenience function to get the automation track
 * of the given type for the channel.
 */
AutomationTrack *
channel_get_automation_track (
  Channel *       channel,
  PortFlags       flags)
{
  Track * track = channel_get_track (channel);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];

      if (at->port_id.flags & flags)
        return at;
    }
  return NULL;
}

/**
 * Clones the channel recursively.
 *
 * Note the given track is not cloned.
 *
 * @param track The track to use for getting the
 *   name.
 */
Channel *
channel_clone (
  Channel * ch,
  Track *   track)
{
  g_return_val_if_fail (track, NULL);

  Channel * clone = channel_new (track);

  /* copy plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->inserts[i])
        {
          Plugin * clone_pl =
            plugin_clone (ch->inserts[i]);
          channel_add_plugin (
            clone, PLUGIN_SLOT_INSERT,
            i, clone_pl, F_NO_CONFIRM,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
        }
    }
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->midi_fx[i])
        {
          Plugin * clone_pl =
            plugin_clone (ch->midi_fx[i]);
          channel_add_plugin (
            clone, PLUGIN_SLOT_MIDI_FX,
            i, clone_pl, F_NO_CONFIRM,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
            F_NO_PUBLISH_EVENTS);
        }
    }
  if (ch->instrument)
    {
      Plugin * clone_pl =
        plugin_clone (ch->instrument);
      channel_add_plugin (
        clone, PLUGIN_SLOT_INSTRUMENT,
        0, clone_pl, F_NO_CONFIRM,
        F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH,
        F_NO_PUBLISH_EVENTS);
    }

  clone->fader.track_pos = clone->track_pos;
  clone->prefader.track_pos = clone->track_pos;
  fader_copy_values (&ch->fader, &clone->fader);

  /* TODO clone port connections, same for
   * plugins */

  return clone;
}

/**
 * Removes the AutomationTrack's associated with
 * this channel from the AutomationTracklist in the
 * corresponding Track.
 */
static void
channel_remove_ats_from_automation_tracklist (
  Channel * ch)
{
  Track * track = channel_get_track (ch);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      if (at->port_id.flags &
            PORT_FLAG_CHANNEL_FADER ||
          at->port_id.flags &
            PORT_FLAG_CHANNEL_MUTE ||
          at->port_id.flags &
            PORT_FLAG_STEREO_BALANCE)
        {
          automation_tracklist_remove_at (
            atl, at, F_NO_FREE);
        }
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
          if (channel->inserts[i])
            {
              channel_remove_plugin (
                channel,
                PLUGIN_SLOT_INSERT,
                i, remove_pl, 0,
                F_NO_RECALC_GRAPH);
            }
        }
      FOREACH_STRIP
        {
          if (channel->midi_fx[i])
            {
              channel_remove_plugin (
                channel,
                PLUGIN_SLOT_MIDI_FX,
                i, remove_pl, 0,
                F_NO_RECALC_GRAPH);
            }
        }
    }

  Port * ports[80000];
  int    num_ports = 0;
  channel_append_all_ports (
    channel,
    ports, &num_ports, 0);
  for (int i = 0; i < num_ports; i++)
    {
      port_disconnect_all (ports[i]);
    }

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

  Track * track = channel_get_track (channel);

  track_processor_free_members (&track->processor);

  switch (track->out_signal_type)
    {
    case TYPE_AUDIO:
      port_free (channel->stereo_out->l);
      port_free (channel->stereo_out->r);
      break;
    case TYPE_EVENT:
      port_free (channel->midi_out);
      break;
    default:
      break;
    }

  /* remove automation tracks - they are already
   * free'd in track_free */
  channel_remove_ats_from_automation_tracklist (
    channel);

  if (GTK_IS_WIDGET (channel->widget))
    {
      gtk_widget_destroy (
        GTK_WIDGET (channel->widget));
    }
}

