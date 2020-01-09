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
#include "audio/ext_port.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/modulator.h"
#include "audio/pan.h"
#include "audio/track.h"
#include "audio/track_processor.h"
#include "audio/transport.h"
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
  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel
   * stereo out. disconnect it */
  track_processor_disconnect_from_prefader (
    &ch->track->processor);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (
    &ch->track->processor, pl);

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
  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin.
   * disconnect it */
  track_processor_disconnect_from_plugin (
    &ch->track->processor, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (
    &ch->track->processor, pl);

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
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_processor_disconnect_from_plugin (
    &ch->track->processor, pl);

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
    &ch->track->processor);
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
  track_processor_disconnect_from_plugin (
    &ch->track->processor, pl);

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
    &ch->track->processor, next_pl);
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
  int i,j;
  Track * tr = self->track;
  PortType out_type =
    tr->out_signal_type;

  /* clear buffers */
  track_processor_clear_buffers (
    &tr->processor);
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

  /* copy the cached MIDI events to the MIDI events
   * in the MIDI in port */
  if (tr->in_signal_type == TYPE_EVENT)
    midi_events_dequeue (
      tr->processor.midi_in->midi_events);
}

void
channel_init_loaded (Channel * ch)
{
  g_message ("initing channel");
  g_warn_if_fail (ch->track);

  ch->ats_size = 1;
  ch->ats =
    calloc (1, sizeof (AutomationTrack *));

  for (unsigned int i =
         AUTOMATABLE_TYPE_CHANNEL_FADER;
       i <= AUTOMATABLE_TYPE_CHANNEL_PAN; i++)
    {
      AutomationTrack * at =
        channel_get_automation_track (ch, i);
      g_return_if_fail (at);

      array_double_size_if_full (
        ch->ats, ch->num_ats, ch->ats_size,
        AutomationTrack *);
      array_append (
        ch->ats, ch->num_ats, at);
    }

  /* fader */
  ch->track->processor.track = ch->track;
  ch->prefader.channel = ch;
  ch->fader.channel = ch;

  track_processor_init_loaded (
    &ch->track->processor);
  passthrough_processor_init_loaded (
    &ch->prefader);
  fader_init_loaded (&ch->fader);

  PortType out_type =
    ch->track->out_signal_type;

  switch (out_type)
    {
    case TYPE_EVENT:
      ch->midi_out->midi_events =
        midi_events_new (
          ch->midi_out);
      ch->midi_out->identifier.owner_type =
        PORT_OWNER_TYPE_TRACK;
      break;
    case TYPE_AUDIO:
      break;
    default:
      break;
    }

  /* routing */
  if (ch->output_pos > -1)
    ch->output =
      TRACKLIST->tracks[ch->output_pos];

  /* init plugins */
  Plugin * pl;
  for (unsigned int i = 0; i < STRIP_SIZE; i++)
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
 * Exposes the channel's ports to the backend.
 */
void
channel_expose_ports_to_backend (
  Channel * ch)
{
  Track * tr = ch->track;

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

#ifdef _WIN32
/**
 * Reconnects the given TrackProcessor's midi in.
 */
static void
reconnect_windows_mme_ext_in (
  Channel * ch,
  Port *    in_port)
{
  int i = 0;
  int ret;
  TrackProcessor * processor =
    &ch->track->processor;

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

      for (i = 0; i < num; i++)
        {
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
  TrackProcessor * processor =
    &ch->track->processor;
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

/**
 * Called when the input has changed for Midi,
 * Instrument or Audio tracks.
 */
void
channel_reconnect_ext_input_ports (
  Channel * ch)
{
  if (ch->track->type == TRACK_TYPE_INSTRUMENT ||
      ch->track->type == TRACK_TYPE_MIDI)
    {
      Port * midi_in =
        ch->track->processor.midi_in;

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
#ifdef _WIN32
        case MIDI_BACKEND_WINDOWS_MME:
          reconnect_windows_mme_ext_in (
            ch, midi_in);
          break;
#endif
        default:
          break;
        }
    }
  else if (ch->track->type == TRACK_TYPE_AUDIO)
    {
      /* if the project was loaded with another
       * backend, the port might not be exposed
       * yet, so expose it */
      port_set_expose_to_backend (
        ch->track->processor.stereo_in->l, 1);
      port_set_expose_to_backend (
        ch->track->processor.stereo_in->r, 1);

      if (AUDIO_ENGINE->audio_backend ==
            AUDIO_BACKEND_JACK &&
          AUDIO_ENGINE->midi_backend ==
            MIDI_BACKEND_JACK)
        {
#ifdef HAVE_JACK
          reconnect_jack_ext_in (
            ch, ch->track->processor.stereo_in->l);
          reconnect_jack_ext_in (
            ch, ch->track->processor.stereo_in->r);
#endif
        }
    }
}

/**
 * Adds to (or subtracts from) the pan.
 */
void
channel_add_pan (void * _channel, float pan)
{
  Channel * channel = (Channel *) _channel;

  port_set_control_value (
    channel->fader.pan,
    CLAMP (
      channel->fader.pan->control + pan, 0.f, 1.f),
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
  Track * tr = ch->track;

  /* set default output */
  if (tr->type == TRACK_TYPE_MASTER)
    {
      ch->output = NULL;
      ch->output_pos = -1;
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
      ch->output = P_MASTER_TRACK;
      ch->output_pos = P_MASTER_TRACK->pos;
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
  if (ch->output)
    {
      /* disconnect Channel's output from the
       * current
       * output channel */
      switch (ch->output->in_signal_type)
        {
        case TYPE_AUDIO:
          port_disconnect (
            ch->stereo_out->l,
            ch->output->processor.stereo_in->l);
          port_disconnect (
            ch->stereo_out->r,
            ch->output->processor.stereo_in->r);
          break;
        case TYPE_EVENT:
          port_disconnect (
            ch->midi_out,
            ch->output->processor.midi_in);
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
    _ADD (pl->in_ports[k]); \
  for (k = 0; k < pl->num_out_ports; k++) \
    _ADD (pl->out_ports[k])

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
    ch->fader.amp && ch->fader.pan);
  _ADD (ch->fader.amp);
  _ADD (ch->fader.pan);

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
        TYPE_AUDIO,
        flow,
        str);

      str[10] = '\0';
      strcat (str, " R");
      r = port_new_with_type (
        TYPE_AUDIO,
        flow,
        str);
    }

  port_set_owner_track (
    l, self->track);
  port_set_owner_track (
    r, self->track);

  *sp =
    stereo_ports_new_from_existing (
      l, r);
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

  port_set_owner_track (
    *port, self->track);
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
  track_processor_init (
    &self->track->processor, self->track);

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
  port_set_control_value (
    channel->fader.pan, pan, 0, 0);
}

float
channel_get_pan (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return
    port_get_control_value (channel->fader.pan, 0);
}

static float
get_current_rms (
  Channel * channel,
  int       left)
{
  float rms = 0.f;
  if (channel->track->out_signal_type ==
        TYPE_EVENT)
    {
      Port * port = channel->midi_out;
      int has_midi_events = 0;
      if (port->write_ring_buffers)
        {
          MidiEvent event;
          while (
            zix_ring_read (
              port->midi_ring, &event,
              sizeof (MidiEvent)) > 0)
            {
              has_midi_events = 1;
            }
        }
      else
        {
          has_midi_events =
            g_atomic_int_compare_and_exchange (
              &port->has_midi_events, 1, 0);
        }

      if (has_midi_events)
        {
          channel->widget->last_midi_trigger_time =
            g_get_real_time ();
          return 1.f;
        }
      else
        {
          /** 350 ms */
          static const float MAX_TIME = 350000.f;
          gint64 time_diff =
            g_get_real_time () -
            channel->widget->last_midi_trigger_time;
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
  else if (channel->track->out_signal_type ==
             TYPE_AUDIO)
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
              lv2_gtk_close_ui (lv2_plugin);
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
  plugin_set_channel_and_slot (
    plugin, channel, pos);

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
 * Returns the last Plugin in the strip.
 */
Plugin *
channel_get_last_plugin (
  Channel * self)
{
  for (int i = STRIP_SIZE - 1; i >= 0; i--)
    {
      if (self->plugins[i])
        return self->plugins[i];
    }
  return NULL;
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
  if (connect)
    port_connect (
      AUDIO_ENGINE->midi_editor_manual_press,
      channel->track->processor.midi_in, 1);
  else
    port_disconnect (
      AUDIO_ENGINE->midi_editor_manual_press,
      channel->track->processor.midi_in);

  if (recalc_graph)
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
 * Convenience function to get the automation track
 * of the given type for the channel.
 */
AutomationTrack *
channel_get_automation_track (
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
        channel->track->
          automation_tracklist.ats[i];
      a = at->automatable;

      if (type == a->type)
        return at;
    }
  return NULL;
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
          Plugin * clone_pl =
            plugin_clone (ch->plugins[i]);
          channel_add_plugin (
            clone, i, clone_pl, F_NO_CONFIRM,
            F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH);
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

  Track * track = channel->track;

  track_processor_free_members (
    &track->processor);

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

