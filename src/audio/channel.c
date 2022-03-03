/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex and zrythm dot org>
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

#include "zrythm-config.h"

#include "audio/audio_track.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/engine_jack.h"
#include "audio/engine_rtmidi.h"
#include "audio/ext_port.h"
#include "audio/group_target_track.h"
#include "audio/hardware_processor.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/midi_event.h"
#include "audio/midi_track.h"
#include "audio/router.h"
#include "audio/pan.h"
#include "audio/port_connections_manager.h"
#include "audio/rtmidi_device.h"
#include "audio/track.h"
#include "audio/track_processor.h"
#include "audio/transport.h"
#include "audio/windows_mme_device.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_gtk.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/stoat.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Connect ports in the case of !prev && !next.
 */
NONNULL
static void
connect_no_prev_no_next (
  Channel * ch,
  Plugin *  pl)
{
  g_debug ("connect no prev no next");

  Track * track = channel_get_track (ch);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel
   * stereo out. disconnect it */
  track_processor_disconnect_from_prefader (
    track->processor);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (
    track->processor, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin to stereo out */
  plugin_connect_to_prefader (pl, ch);
}

/**
 * Connect ports in the case of !prev && next.
 */
NONNULL
static void
connect_no_prev_next (
  Channel * ch,
  Plugin *  pl,
  Plugin *  next_pl)
{
  g_debug ("connect no prev next");

  Track * track = channel_get_track (ch);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin.
   * disconnect it */
  track_processor_disconnect_from_plugin (
    track->processor, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (
    track->processor, pl);

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
NONNULL
static void
connect_prev_no_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl)
{
  g_debug ("connect prev no next");

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
NONNULL
static void
connect_prev_next (
  Channel * ch,
  Plugin *  prev_pl,
  Plugin *  pl,
  Plugin *  next_pl)
{
  g_debug ("connect prev next");

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
NONNULL
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
    track->processor, pl);

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
    track->processor);
}

/**
 * Disconnect ports in the case of !prev && next.
 */
NONNULL
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
    track->processor, pl);

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
    track->processor, next_pl);
}

/**
 * Connect ports in the case of prev && !next.
 */
NONNULL
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
NONNULL
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
    tr->processor);
  fader_clear_buffers (
    self->prefader);
  fader_clear_buffers (self->fader);

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

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_prepare_process (self->sends[i]);
    }

  if (tr->in_signal_type == TYPE_EVENT)
    {
#ifdef HAVE_RTMIDI
      /* extract the midi events from the ring
       * buffer */
      if (midi_backend_is_rtmidi (
            AUDIO_ENGINE->midi_backend))
        {
          port_prepare_rtmidi_events (
            tr->processor->midi_in);
        }
#endif

      /* copy the cached MIDI events to the
       * MIDI events in the MIDI in port */
      midi_events_dequeue (
        tr->processor->midi_in->midi_events);
    }
}

void
channel_init_loaded (
  Channel * self,
  Track *   track)
{
  g_debug ("initing channel");

  self->track = track;
  self->magic = CHANNEL_MAGIC;

  /* fader */
  self->prefader->track = track;
  self->fader->track = track;

  fader_init_loaded (
    self->prefader, track, NULL, NULL);
  fader_init_loaded (
    self->fader, track, NULL, NULL);

  PortType out_type =
    track->out_signal_type;

  switch (out_type)
    {
    case TYPE_EVENT:
      self->midi_out->midi_events =
        midi_events_new ();
      break;
    case TYPE_AUDIO:
      /* make sure master is exposed to backend */
      if (track->type == TRACK_TYPE_MASTER)
        {
          self->stereo_out->l->exposed_to_backend =
            true;
          self->stereo_out->r->exposed_to_backend =
            true;
        }
      break;
    default:
      break;
    }

#define INIT_PLUGIN(pl,_slot,_slot_type) \
  if (pl) \
    { \
      pl->id.track_name_hash = \
        track_get_name_hash (self->track); \
      pl->id.slot = _slot; \
      pl->id.slot_type = _slot_type; \
      plugin_init_loaded ( \
        pl, self->track, NULL); \
    }

  /* init plugins */
  Plugin * pl;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      pl = self->inserts[i];
      INIT_PLUGIN (pl, i, PLUGIN_SLOT_INSERT);
      pl = self->midi_fx[i];
      INIT_PLUGIN (pl, i, PLUGIN_SLOT_MIDI_FX);
    }
  if (self->instrument)
    {
      pl = self->instrument;
      INIT_PLUGIN (pl, -1, PLUGIN_SLOT_INSTRUMENT);
    }

#undef INIT_PLUGIN

  /* init sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_init_loaded (
        self->sends[i], self->track);
    }
}

void
channel_set_magic (
  Channel * self)
{
  Plugin * plugins[120];
  int num_plugins =
    channel_get_plugins (self, plugins);

  for (int i = 0; i < num_plugins; i++)
    {
      Plugin * pl = plugins[i];
      pl->magic = PLUGIN_MAGIC;
    }
}

/**
 * Exposes the channel's ports to the backend.
 */
void
channel_expose_ports_to_backend (
  Channel * ch)
{
  Track * tr = channel_get_track (ch);

  /* skip if auditioner */
  if (track_is_auditioner (tr))
    return;

  g_message ("%s: %s", __func__, tr->name);

  if (tr->in_signal_type ==
        TYPE_AUDIO)
    {
      port_set_expose_to_backend (
        tr->processor->stereo_in->l , true);
      port_set_expose_to_backend (
        tr->processor->stereo_in->r , true);
    }
  if (tr->in_signal_type ==
        TYPE_EVENT)
    {
      port_set_expose_to_backend (
        tr->processor->midi_in , true);
    }
  if (tr->out_signal_type ==
        TYPE_AUDIO)
    {
      port_set_expose_to_backend (
        ch->stereo_out->l , true);
      port_set_expose_to_backend (
        ch->stereo_out->r , true);
    }
  if (tr->out_signal_type ==
        TYPE_EVENT)
    {
      port_set_expose_to_backend (
        ch->midi_out , true);
    }
}

/**
 * Called when the input has changed for Midi,
 * Instrument or Audio tracks.
 */
void
channel_reconnect_ext_input_ports (
  Channel * self)
{
  Track * track = channel_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  /* skip if auditioner track */
  if (track_is_auditioner (track))
    return;

  g_return_if_fail (
    channel_is_in_active_project (self));

  if (track->type == TRACK_TYPE_INSTRUMENT
      || track->type == TRACK_TYPE_MIDI
      || track->type == TRACK_TYPE_CHORD)
    {
      Port * midi_in = track->processor->midi_in;

      /* if the project was loaded with another
       * backend, the port might not be exposed
       * yet, so expose it */
      port_set_expose_to_backend (midi_in, 1);

      /* disconnect all connections to hardware */
      port_disconnect_hw_inputs (midi_in);

      if (self->all_midi_ins)
        {
          for (int i = 0;
               i <
                 HW_IN_PROCESSOR->
                   num_selected_midi_ports;
               i++)
            {
              char * port_id =
                HW_IN_PROCESSOR->
                  selected_midi_ports[i];
              Port * source =
                hardware_processor_find_port (
                  HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_message (
                    "port for %s not found",
                    port_id);
                  continue;
                }
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR,
                &source->id, &midi_in->id,
                1.f, F_LOCKED, F_ENABLE);
            }
        }
      else
        {
          for (int i = 0;
               i < self->num_ext_midi_ins; i++)
            {
              char * port_id =
                ext_port_get_id (
                  self->ext_midi_ins[i]);
              Port * source =
                hardware_processor_find_port (
                  HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_message (
                    "port for %s not found",
                    port_id);
                  g_free (port_id);
                  continue;
                }
              g_free (port_id);
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR,
                &source->id, &midi_in->id,
                1.f, F_LOCKED, F_ENABLE);
            }
        }
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      /* if the project was loaded with another
       * backend, the port might not be exposed
       * yet, so expose it */
      Port * l =
        track->processor->stereo_in->l;
      Port * r =
        track->processor->stereo_in->r;
      port_set_expose_to_backend (l, true);
      port_set_expose_to_backend (r, true);

      /* disconnect all connections to hardware */
      port_disconnect_hw_inputs (l);
      port_disconnect_hw_inputs (r);

      for (int i = 0;
           i < self->num_ext_stereo_l_ins; i++)
        {
          char * port_id =
            ext_port_get_id (
              self->ext_stereo_l_ins[i]);
          Port * source =
            hardware_processor_find_port (
              HW_IN_PROCESSOR, port_id);
          if (!source)
            {
              g_warning (
                "port for %s not found",
                port_id);
              g_free (port_id);
              continue;
            }
          g_free (port_id);
          port_connections_manager_ensure_connect (
            PORT_CONNECTIONS_MGR,
            &source->id, &l->id,
            1.f, F_LOCKED, F_ENABLE);
        }
      for (int i = 0;
           i < self->num_ext_stereo_r_ins; i++)
        {
          char * port_id =
            ext_port_get_id (
              self->ext_stereo_r_ins[i]);
          Port * source =
            hardware_processor_find_port (
              HW_IN_PROCESSOR, port_id);
          if (!source)
            {
              g_warning (
                "port for %s not found",
                port_id);
              g_free (port_id);
              continue;
            }
          g_free (port_id);
          port_connections_manager_ensure_connect (
            PORT_CONNECTIONS_MGR,
            &source->id, &r->id,
            1.f, F_LOCKED, F_ENABLE);
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
    channel->fader->balance,
    CLAMP (
      channel->fader->balance->control + pan,
      0.f, 1.f),
    0, 0);
}


/**
 * Sets fader to 0.0.
 */
void
channel_reset_fader (
  Channel * self,
  bool      fire_events)
{
  fader_set_amp (self->fader, 1.0f);

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_CHANNEL_FADER_VAL_CHANGED, self);
    }
}

/**
 * Gets whether mono compatibility is enabled.
 */
bool
channel_get_mono_compat_enabled (
  Channel * self)
{
  return
    fader_get_mono_compat_enabled (self->fader);
}

/**
 * Sets whether mono compatibility is enabled.
 */
void
channel_set_mono_compat_enabled (
  Channel * self,
  bool      enabled,
  bool      fire_events)
{
  fader_set_mono_compat_enabled (
    self->fader, enabled, fire_events);
}

static void
channel_connect_plugins (
  Channel * self)
{
  g_return_if_fail (
    channel_is_in_active_project (self));

  for (int i = 0; i < 3; i++)
    {
      PluginSlotType slot_type;
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = NULL;
          int slot = j;
          if (i == 0)
            {
              slot_type = PLUGIN_SLOT_MIDI_FX;
              plugin = self->midi_fx[j];
            }
          else if (i == 1)
            {
              slot_type = PLUGIN_SLOT_INSTRUMENT;
              plugin = self->instrument;
            }
          else
            {
              slot_type = PLUGIN_SLOT_INSERT;
              plugin = self->inserts[j];
            }
          if (!plugin)
            continue;

          if (!plugin->instantiated
              && !plugin->instantiation_failed)
            {
              /* TODO handle error */
              plugin_instantiate (
                plugin, NULL, NULL);
            }

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
              g_return_if_reached ();
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
              g_return_if_reached ();
            }

          Plugin * next_pl = NULL;
          for (int k =
                 (slot_type == PLUGIN_SLOT_INSTRUMENT ?
                    0 : slot + 1); k < STRIP_SIZE; k++)
            {
              next_pl = next_plugins[k];
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
                  for (int k = 0; k < STRIP_SIZE; k++)
                    {
                      next_pl = self->inserts[k];
                      if (next_pl)
                        break;
                    }
                }
            }

          Plugin * prev_pl = NULL;
          if (slot_type != PLUGIN_SLOT_INSTRUMENT)
            {
              for (int k = slot - 1; k >= 0; k--)
                {
                  prev_pl = prev_plugins[k];
                  if (prev_pl)
                    break;
                }
            }
          if (!prev_pl &&
              slot_type == PLUGIN_SLOT_INSERT)
            {
              if (self->instrument)
                prev_pl = self->instrument;
              else
                {
                  for (int k = STRIP_SIZE - 1; k >= 0; k--)
                    {
                      prev_pl = self->midi_fx[k];
                      if (prev_pl)
                        break;
                    }
                }
            }

          if (prev_pl
              && !prev_pl->instantiated
              && !prev_pl->instantiation_failed)
            {
              plugin_instantiate (
                prev_pl, NULL, NULL);
            }
          if (next_pl
              && !next_pl->instantiated
              && !next_pl->instantiation_failed)
            {
              plugin_instantiate (
                next_pl, NULL, NULL);
            }

          if (!prev_pl && !next_pl)
            {
              connect_no_prev_no_next (
                self, plugin);
            }
          else if (!prev_pl && next_pl)
            {
              connect_no_prev_next (
                self, plugin, next_pl);
            }
          else if (prev_pl && !next_pl)
            {
              connect_prev_no_next (
                self, prev_pl, plugin);
            }
          else if (prev_pl && next_pl)
            {
              connect_prev_next (
                self, prev_pl, plugin, next_pl);
            }

        } /* end for each slot */

    } /* end for each slot type */
}

/**
 * Connects the channel's ports.
 *
 * This should only be called on project tracks.
 */
void
channel_connect (
  Channel * ch)
{
  Track * tr = channel_get_track (ch);
  g_return_if_fail (tr);

  g_return_if_fail (
    track_is_in_active_project (tr)
    || track_is_auditioner (tr));

  g_message ("connecting channel...");

  /* set default output */
  if (tr->type == TRACK_TYPE_MASTER
      && !track_is_auditioner (tr))
    {
      ch->output_name_hash = 0;
      ch->has_output = 0;
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->stereo_out->l->id,
        &MONITOR_FADER->stereo_in->l->id,
        1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->stereo_out->r->id,
        &MONITOR_FADER->stereo_in->r->id,
        1.f, F_LOCKED, F_ENABLE);
    }

  if (tr->out_signal_type == TYPE_AUDIO)
    {
      /* connect stereo in to stereo out through
       * fader */
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->prefader->stereo_out->l->id,
        &ch->fader->stereo_in->l->id,
        1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->prefader->stereo_out->r->id,
        &ch->fader->stereo_in->r->id,
        1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->fader->stereo_out->l->id,
        &ch->stereo_out->l->id,
        1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->fader->stereo_out->r->id,
        &ch->stereo_out->r->id,
        1.f, F_LOCKED, F_ENABLE);
    }
  else if (tr->out_signal_type == TYPE_EVENT)
    {
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->prefader->midi_out->id,
        &ch->fader->midi_in->id,
        1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR,
        &ch->fader->midi_out->id,
        &ch->midi_out->id,
        1.f, F_LOCKED, F_ENABLE);
    }

  /** Connect MIDI in and piano roll to MIDI
   * out. */
  track_processor_connect_to_prefader (
    tr->processor);

  /* connect plugins */
  channel_connect_plugins (ch);

  /* expose ports to backend */
  if (AUDIO_ENGINE && AUDIO_ENGINE->setup)
    {
      channel_expose_ports_to_backend (ch);
    }

  /* connect sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * send = ch->sends[i];
      channel_send_connect_to_owner (send);
    }

  /* connect the designated midi inputs */
  channel_reconnect_ext_input_ports (ch);

  g_message ("done connecting channel");
}

Track *
channel_get_track (
  Channel * self)
{
  if (G_LIKELY (self->track))
    return self->track;
  else
    {
      g_return_val_if_fail (
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
  if (!self->has_output)
    return NULL;

  Track * track = channel_get_track (self);
  g_return_val_if_fail (track, NULL);
  Tracklist * tracklist =
    track_get_tracklist (track);
  Track * output_track =
    tracklist_find_track_by_name_hash (
    tracklist, self->output_name_hash);
  g_return_val_if_fail (
    output_track && track != output_track, NULL);

  return output_track;
}

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 */
void
channel_append_ports (
  Channel *   self,
  GPtrArray * ports,
  bool        include_plugins)
{
  g_return_if_fail (self->track);

#define _ADD(port) \
  g_warn_if_fail (port); \
  g_ptr_array_add (ports, port)

  /* add channel ports */
  if (self->track->out_signal_type == TYPE_AUDIO)
    {
      _ADD (self->stereo_out->l);
      _ADD (self->stereo_out->r);

      /* add fader ports */
      _ADD (self->fader->stereo_in->l);
      _ADD (self->fader->stereo_in->r);
      _ADD (self->fader->stereo_out->l);
      _ADD (self->fader->stereo_out->r);

      /* add prefader ports */
      _ADD (self->prefader->stereo_in->l);
      _ADD (self->prefader->stereo_in->r);
      _ADD (self->prefader->stereo_out->l);
      _ADD (self->prefader->stereo_out->r);
    }
  else if (self->track->out_signal_type ==
             TYPE_EVENT)
    {
      _ADD (self->midi_out);

      /* add fader ports */
      _ADD (self->fader->midi_in);
      _ADD (self->fader->midi_out);

      /* add prefader ports */
      _ADD (self->prefader->midi_in);
      _ADD (self->prefader->midi_out);
    }

  /* add fader amp and balance control */
  _ADD (self->prefader->amp);
  _ADD (self->prefader->balance);
  _ADD (self->prefader->mute);
  _ADD (self->prefader->solo);
  _ADD (self->prefader->listen);
  _ADD (self->prefader->mono_compat_enabled);
  _ADD (self->fader->amp);
  _ADD (self->fader->balance);
  _ADD (self->fader->mute);
  _ADD (self->fader->solo);
  _ADD (self->fader->listen);
  _ADD (self->fader->mono_compat_enabled);

  if (include_plugins)
    {
#define ADD_PLUGIN_PORTS(x) \
  if (x) \
    plugin_append_ports (x, ports)

      /* add plugin ports */
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          ADD_PLUGIN_PORTS (self->inserts[j]);
          ADD_PLUGIN_PORTS (self->midi_fx[j]);
        }

      ADD_PLUGIN_PORTS (self->instrument);
    }

#undef ADD_PLUGIN_PORTS

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_append_ports (
        self->sends[i], ports);
    }
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
  bool      loading)
{
  if (loading)
    {
      return;
    }

  char str[80];
  strcpy (str, "Stereo out");

  strcat (str, " L");
  Port * l =
    port_new_with_type_and_owner (
      TYPE_AUDIO, FLOW_OUTPUT, str,
      PORT_OWNER_TYPE_CHANNEL, self);

  str[10] = '\0';
  strcat (str, " R");
  Port * r =
    port_new_with_type_and_owner (
      TYPE_AUDIO, FLOW_OUTPUT, str,
      PORT_OWNER_TYPE_CHANNEL, self);

  self->stereo_out =
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
    port_new_with_type_and_owner (
      TYPE_EVENT, flow, str,
      PORT_OWNER_TYPE_CHANNEL, self);
}

/**
 * Creates a channel of the given type with the
 * given label.
 */
Channel *
channel_new (
  Track *     track)
{
  Channel * self = object_new (Channel);

  track->channel = self;
  self->schema_version =
    CHANNEL_SCHEMA_VERSION;
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
      init_stereo_out_ports (self, 0);
      break;
    case TYPE_EVENT:
      init_midi_port (self, 0);
      break;
    default:
      break;
    }

  FaderType fader_type =
    track_get_fader_type (track);
  FaderType prefader_type =
    track_type_get_prefader_type (track->type);
  self->fader =
    fader_new (
      fader_type, false, track, NULL, NULL);
  self->prefader =
    fader_new (
      prefader_type, true, track, NULL, NULL);

  /* init sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      self->sends[i] =
        channel_send_new (
          track_get_name_hash (track), i, track);
      self->sends[i]->track = track;
    }

  return self;
}

void
channel_set_phase (void * _channel, float phase)
{
  Channel * channel = (Channel *) _channel;
  channel->fader->phase = phase;

  /* FIXME use an event */
  /*if (channel->widget)*/
    /*gtk_label_set_text (channel->widget->phase_reading,*/
                        /*g_strdup_printf ("%.1f", phase));*/
}

float
channel_get_phase (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader->phase;
}

void
channel_set_balance_control (
  void * _channel,
  float pan)
{
  Channel * channel = (Channel *) _channel;
  port_set_control_value (
    channel->fader->balance, pan, 0, 0);
}

float
channel_get_balance_control (
  void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return
    port_get_control_value (
      channel->fader->balance, 0);
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
    {
      disconnect_no_prev_no_next (ch, pl);
    }
  else if (!prev_plugin && next_plugin)
    {
      disconnect_no_prev_next (ch, pl, next_plugin);
    }
  else if (prev_plugin && !next_plugin)
    {
      disconnect_prev_no_next (ch, prev_plugin, pl);
    }
  else if (prev_plugin && next_plugin)
    {
      disconnect_prev_next (
        ch, prev_plugin, pl, next_plugin);
    }

  /* unexpose all JACK ports */
  plugin_expose_ports (
    pl, F_NOT_EXPOSE, true, true);
}

/**
 * Removes a plugin at pos from the channel.
 *
 * @param moving_plugin Whether or not we are
 *   moving the plugin.
 * @param deleting_plugin Whether or not we are
 *   deleting the plugin.
 * @param deleting_channel If true, the automation
 *   tracks associated with the plugin are not
 *   deleted at this time.
 * @param recalc_graph Recalculate mixer graph.
 */
void
channel_remove_plugin (
  Channel *      channel,
  PluginSlotType slot_type,
  int            slot,
  bool           moving_plugin,
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
    case PLUGIN_SLOT_INSTRUMENT:
      plugin = channel->instrument;
      break;
    default:
      break;
    }
  g_return_if_fail (
    IS_PLUGIN_AND_NONNULL (plugin));
  g_return_if_fail (
    plugin->id.track_name_hash ==
      track_get_name_hash (channel->track));

  Track * track = channel_get_track (channel);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));
  g_message (
    "Removing %s from %s:%s:%d",
    plugin->setting->descr->name, track->name,
    plugin_slot_type_strings[slot_type].str, slot);

  /* if moving, the move is already handled in
   * plugin_move_automation() inside
   * plugin_move(). */
  if (!moving_plugin)
    {
      plugin_remove_ats_from_automation_tracklist (
        plugin, deleting_plugin,
        !deleting_channel && !deleting_plugin);
    }

  if (channel_is_in_active_project (channel))
    channel_disconnect_plugin_from_strip (
      channel, slot, plugin);

  /* if deleting plugin disconnect the plugin
   * entirely */
  if (deleting_plugin)
    {
      if (channel_is_in_active_project (channel)
          && plugin_is_selected (plugin))
        {
          mixer_selections_remove_slot (
            MIXER_SELECTIONS, plugin->id.slot,
            slot_type, F_PUBLISH_EVENTS);
        }

      plugin_disconnect (plugin);
      object_free_w_func_and_null (
        plugin_free, plugin);
    }

  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      channel->inserts[slot] = NULL;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      channel->midi_fx[slot] = NULL;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      channel->instrument = NULL;
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

  if (track_is_in_active_project (track)
      && !track->disconnecting
      /* only verify if we are deleting the plugin.
       * if the plugin is moved to another slot
       * this check fails because the port
       * identifiers in the automation tracks are
       * already updated to point to the next
       * slot and the plugin is not found there
       * yet */
      && deleting_plugin
      && !moving_plugin)
    {
      track_validate (channel->track);
    }

  if (recalc_graph)
    router_recalc_graph (ROUTER, F_NOT_SOFT);
}

/**
 * Adds given plugin to given position in the
 * strip.
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
 * @param moving_plugin Whether or not we are
 *   moving the plugin.
 * @param gen_automatables Generatate plugin
 *   automatables.
 *   To be used when creating a new plugin only.
 * @param recalc_graph Recalculate mixer graph.
 * @param pub_events Publish events.
 *
 * @return true if plugin added, false if not.
 */
bool
channel_add_plugin (
  Channel * self,
  PluginSlotType slot_type,
  int       slot,
  Plugin *  plugin,
  bool      confirm,
  bool      moving_plugin,
  bool      gen_automatables,
  bool      recalc_graph,
  bool      pub_events)
{
  g_return_val_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (
      slot_type, slot),
    false);

  Track * track = channel_get_track (self);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), 0);
  bool prev_enabled = track->enabled;
  track->enabled = false;

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
  if (slot_type == PLUGIN_SLOT_INSTRUMENT)
    existing_pl = self->instrument;
  else
    existing_pl = plugins[slot];

  if (existing_pl)
    {
      g_message (
        "existing plugin exists at %s:%d",
        track->name, slot);
    }
  /* TODO move confirmation to widget */
#if 0
  /* confirm if another plugin exists */
  if (confirm && existing_pl && ZRYTHM_HAVE_UI)
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
#endif

  /* free current plugin */
  if (existing_pl)
    {
      channel_remove_plugin (
        self, slot_type, slot,
        moving_plugin, F_DELETING_PLUGIN,
        F_NOT_DELETING_CHANNEL,
        F_NO_RECALC_GRAPH);
    }

  g_message (
    "Inserting %s %s at %s:%s:%d",
    plugin_slot_type_to_string (slot_type),
    plugin->setting->descr->name, track->name,
    plugin_slot_type_strings[slot_type].str, slot);
  if (slot_type == PLUGIN_SLOT_INSTRUMENT)
    {
      self->instrument = plugin;
    }
  else
    {
      plugins[slot] = plugin;
    }
  plugin->track = track;
  plugin_set_track_and_slot (
    plugin,
    track_get_name_hash (self->track),
    slot_type, slot);
  g_return_val_if_fail (plugin->track, false);

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
  for (int i =
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
          for (int i = 0; i < STRIP_SIZE; i++)
            {
              next_pl = self->inserts[i];
              if (next_pl)
                break;
            }
        }
    }

  Plugin * prev_pl = NULL;
  if (slot_type != PLUGIN_SLOT_INSTRUMENT)
    {
      for (int i = slot - 1; i >= 0; i--)
        {
          prev_pl = prev_plugins[i];
          if (prev_pl)
            break;
        }
    }
  if (!prev_pl &&
      slot_type == PLUGIN_SLOT_INSERT)
    {
      if (self->instrument)
        prev_pl = self->instrument;
      else
        {
          for (int i = STRIP_SIZE - 1; i >= 0; i--)
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

  g_debug (
    "%s: connecting plugin ports...", __func__);

  if (track_is_in_active_project (track))
    {
      channel_connect_plugins (self);
    }

  track->enabled = prev_enabled;

  if (gen_automatables)
    {
      plugin_generate_automation_tracks (
        plugin, track);
    }

  if (pub_events)
    {
      EVENTS_PUSH (ET_PLUGIN_ADDED, plugin);
    }

  if (recalc_graph)
    {
      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  return 1;
}

/**
 * Updates the track name hash in the channel and
 * all related ports and identifiers.
 */
NONNULL
void
channel_update_track_name_hash (
  Channel *    self,
  unsigned int old_name_hash,
  unsigned int new_name_hash)
{
  Track * track = self->track;
  g_return_if_fail (track);

  /* update output */
  if (track_is_in_active_project (track)
      && self->has_output)
    {
      Track * out_track =
        channel_get_output_track (self);
      g_return_if_fail (
        IS_TRACK_AND_NONNULL (out_track));
      int child_idx =
        group_target_track_find_child (
          out_track, old_name_hash);
      g_return_if_fail (child_idx >= 0);

      out_track->children[child_idx] =
        new_name_hash;
      g_debug (
        "%s: setting output of track '%s' to '%s'",
        __func__, self->track->name,
        out_track->name);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * send = self->sends[i];
      send->track_name_hash = new_name_hash;
    }

#define SET_PLUGIN_NAME_HASH(pl) \
  if (pl) \
    plugin_set_track_name_hash ( \
      pl, new_name_hash)

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      SET_PLUGIN_NAME_HASH (self->inserts[i]);
      SET_PLUGIN_NAME_HASH (self->midi_fx[i]);
    }
  SET_PLUGIN_NAME_HASH (self->instrument);
}

/**
 * Convenience function to get the automation track
 * of the given type for the channel.
 */
AutomationTrack *
channel_get_automation_track (
  Channel *       channel,
  PortFlags       port_flags)
{
  Track * track = channel_get_track (channel);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];

      if (at->port_id.flags & port_flags)
        return at;
    }
  return NULL;
}

/**
 * Clones the channel recursively.
 *
 * @note The given track is not cloned.
 *
 * @param error To be filled if an error occurred.
 * @param track The track to use for getting the
 *   name.
 */
Channel *
channel_clone (
  Channel * ch,
  Track *   track,
  GError ** error)
{
  g_return_val_if_fail (!error || !*error, NULL);

  Channel * clone = channel_new (track);

  clone->fader->track = clone->track;
  clone->prefader->track = clone->track;
  fader_copy_values (ch->fader, clone->fader);

  clone->has_output = ch->has_output;
  clone->output_name_hash = ch->output_name_hash;

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * src_send = ch->sends[i];
      channel_send_copy_values (
        clone->sends[i], src_send);
    }

#define CLONE_AND_ADD_PL(pl,slot_type,slot) \
  { \
    GError * err = NULL; \
    Plugin * clone_pl = \
      plugin_clone (pl, &err); \
    if (!clone_pl) \
      { \
        PROPAGATE_PREFIXED_ERROR ( \
          error, err, "%s", \
          _("Failed to clone plugin")); \
        object_free_w_func_and_null ( \
          channel_free, clone); \
        return NULL; \
      } \
    channel_add_plugin ( \
      clone, slot_type, \
      slot, clone_pl, F_NO_CONFIRM, \
      F_NOT_MOVING_PLUGIN, \
      F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH, \
      F_NO_PUBLISH_EVENTS); \
  }

  /* copy plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->inserts[i])
        {
          CLONE_AND_ADD_PL (
            ch->inserts[i], PLUGIN_SLOT_INSERT, i);
        }
    }
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->midi_fx[i])
        {
          CLONE_AND_ADD_PL (
            ch->midi_fx[i], PLUGIN_SLOT_MIDI_FX,
            i);
        }
    }
  if (ch->instrument)
    {
      CLONE_AND_ADD_PL (
        ch->instrument, PLUGIN_SLOT_INSTRUMENT,
        -1);
    }

#undef CLONE_AND_ADD_PL

  /* copy port connection info (including
   * plugins) */
  GPtrArray * ports = g_ptr_array_new ();
  channel_append_ports (ch, ports, true);
  GPtrArray * ports_clone = g_ptr_array_new ();
  channel_append_ports (ch, ports_clone, true);
  g_return_val_if_fail (
    ports->len == ports_clone->len, NULL);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      Port * clone_port =
        g_ptr_array_index (ports_clone, i);
      g_return_val_if_fail (
        port_identifier_is_equal (
          &port->id, &clone_port->id),
        NULL);
      port_copy_values (clone_port, port);
    }
  g_ptr_array_unref (ports);
  g_ptr_array_unref (ports_clone);

  return clone;
}

/**
 * Selects/deselects all plugins in the given slot
 * type.
 */
void
channel_select_all (
  Channel *      self,
  PluginSlotType type,
  bool           select)
{
  mixer_selections_clear (
    MIXER_SELECTIONS, F_PUBLISH_EVENTS);
  if (!select)
    return;

  switch (type)
    {
    case PLUGIN_SLOT_INSERT:
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          if (self->inserts[i])
            {
              plugin_select (
                self->inserts[i], F_SELECT,
                F_NOT_EXCLUSIVE);
            }
        }
      break;
    case PLUGIN_SLOT_MIDI_FX:
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          if (self->midi_fx[i])
            {
              plugin_select (
                self->midi_fx[i], F_SELECT,
                F_NOT_EXCLUSIVE);
            }
        }
      break;
    default:
      g_warning ("not implemented");
      break;
    }
}

int
channel_get_plugins (
  Channel * ch,
  Plugin ** pls)
{
  int size = 0;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->inserts[i])
        {
          pls[size++] = ch->inserts[i];
        }
    }
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (ch->midi_fx[i])
        {
          pls[size++] = ch->midi_fx[i];
        }
    }
  if (ch->instrument)
    {
      pls[size++] = ch->instrument;
    }

  return size;
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
  Channel * self,
  bool      remove_pl)
{
  if (remove_pl)
    {
      FOREACH_STRIP
        {
          if (self->inserts[i])
            {
              channel_remove_plugin (
                self,
                PLUGIN_SLOT_INSERT,
                i, F_NOT_MOVING_PLUGIN,
                remove_pl, false,
                F_NO_RECALC_GRAPH);
            }
          if (self->midi_fx[i])
            {
              channel_remove_plugin (
                self,
                PLUGIN_SLOT_MIDI_FX,
                i, F_NOT_MOVING_PLUGIN,
                remove_pl, false,
                F_NO_RECALC_GRAPH);
            }
        }
      if (self->instrument)
        {
          channel_remove_plugin (
            self,
            PLUGIN_SLOT_INSTRUMENT,
            0, F_NOT_MOVING_PLUGIN,
            remove_pl, false,
            F_NO_RECALC_GRAPH);
        }
    }

  /* disconnect from output */
  if (channel_is_in_active_project (self) &&
      self->has_output)
    {
      Track * out_track =
        channel_get_output_track (self);
      g_return_if_fail (
        IS_TRACK_AND_NONNULL (out_track));
      group_target_track_remove_child (
        out_track,
        track_get_name_hash (self->track),
        F_DISCONNECT,
        F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
    }

  /* disconnect fader/prefader */
  fader_disconnect_all (self->prefader);
  fader_disconnect_all (self->fader);

  /* disconnect all ports */
  GPtrArray * ports = g_ptr_array_new ();
  channel_append_ports (self, ports, true);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      if (!IS_PORT (port)
          ||
          port_is_in_active_project (port) !=
            track_is_in_active_project (
              self->track))
        {
          g_critical ("invalid port");
          g_ptr_array_unref (ports);
          return;
        }

      port_disconnect_all (port);
    }
  g_ptr_array_unref (ports);
}

/**
 * Frees the channel.
 *
 * Channels should never be free'd by themselves
 * in normal circumstances. Use track_free to
 * free them.
 */
void
channel_free (Channel * self)
{
  object_free_w_func_and_null (
    fader_free, self->prefader);
  object_free_w_func_and_null (
    fader_free, self->fader);

  object_free_w_func_and_null (
    stereo_ports_free, self->stereo_out);
  object_free_w_func_and_null (
    port_free, self->midi_out);

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * send = self->sends[i];
      object_free_w_func_and_null (
        channel_send_free, send);
    }

  object_zero_and_free (self);
}
