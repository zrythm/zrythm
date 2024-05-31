// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <cstdlib>
#include <utility>

#include "dsp/audio_track.h"
#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel.h"
#include "dsp/ext_port.h"
#include "dsp/group_target_track.h"
#include "dsp/hardware_processor.h"
#include "dsp/instrument_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/midi_track.h"
#include "dsp/port.h"
#include "dsp/port_connections_manager.h"
#include "dsp/position.h"
#include "dsp/router.h"
#include "dsp/rtmidi_device.h"
#include "dsp/track.h"
#include "dsp/track_processor.h"
#include "dsp/windows_mme_device.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/dialogs.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"
#include "plugin.h"
#include "plugin_identifier.h"
#include <unistd.h>

typedef enum
{
  Z_DSP_CHANNEL_ERROR_FAILED,
} ZDspChannelError;

#define Z_DSP_CHANNEL_ERROR z_dsp_channel_error_quark ()
GQuark
z_dsp_channel_error_quark (void);
G_DEFINE_QUARK (
  z - dsp - channel - error - quark,
  z_dsp_channel_error)

bool
Channel::is_in_active_project ()
{
  return track_is_in_active_project (&track_);
}

/**
 * Connect ports in the case of !prev && !next.
 */
NONNULL static void
connect_no_prev_no_next (Channel * ch, Plugin * pl)
{
  g_debug ("connect no prev no next");

  Track &track = ch->get_track ();

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel
   * stereo out. disconnect it */
  track_processor_disconnect_from_prefader (track.processor);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (track.processor, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin to stereo out */
  plugin_connect_to_prefader (pl, ch);
}

/**
 * Connect ports in the case of !prev && next.
 */
NONNULL static void
connect_no_prev_next (Channel * ch, Plugin * pl, Plugin * next_pl)
{
  g_debug ("connect no prev next");

  Track &track = ch->get_track ();

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin.
   * disconnect it */
  track_processor_disconnect_from_plugin (track.processor, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_processor_connect_to_plugin (track.processor, pl);

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
NONNULL static void
connect_prev_no_next (Channel * ch, Plugin * prev_pl, Plugin * pl)
{
  g_debug ("connect prev no next");

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to channel stereo out.
   * disconnect it */
  plugin_disconnect_from_prefader (prev_pl, ch);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's outs to
   * plugin */
  plugin_connect_to_plugin (prev_pl, pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin output ports to stereo_out */
  plugin_connect_to_prefader (pl, ch);
}

/**
 * Connect ports in the case of prev && next.
 */
NONNULL static void
connect_prev_next (Channel * ch, Plugin * prev_pl, Plugin * pl, Plugin * next_pl)
{
  g_debug ("connect prev next");

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to the next pl.
   * disconnect them */
  plugin_disconnect_from_plugin (prev_pl, next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to
   * plugin */
  plugin_connect_to_plugin (prev_pl, pl);

  /* ------------------------------------------
   * Connect output ports
   * ------------------------------------------ */

  /* connect plugin's audio outs to next
   * plugin */
  plugin_connect_to_plugin (pl, next_pl);
}

/**
 * Disconnect ports in the case of !prev && !next.
 */
NONNULL static void
disconnect_no_prev_no_next (Channel * ch, Plugin * pl)
{
  Track &track = ch->get_track ();

  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_processor_disconnect_from_plugin (track.processor, pl);

  /* --------------------------------------
   * disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin from stereo out */
  plugin_disconnect_from_prefader (pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to
   * channel stereo out. connect it */
  track_processor_connect_to_prefader (track.processor);
}

/**
 * Disconnect ports in the case of !prev && next.
 */
NONNULL static void
disconnect_no_prev_next (Channel * ch, Plugin * pl, Plugin * next_pl)
{
  Track &track = ch->get_track ();

  /* -------------------------------------------
   * Disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_processor_disconnect_from_plugin (track.processor, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin's midi & audio outs from next
   * plugin */
  plugin_disconnect_from_plugin (pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to next
   * plugin. connect it */
  track_processor_connect_to_plugin (track.processor, next_pl);
}

/**
 * Connect ports in the case of prev && !next.
 */
NONNULL static void
disconnect_prev_no_next (Channel * ch, Plugin * prev_pl, Plugin * pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from
   * plugin */
  plugin_disconnect_from_plugin (prev_pl, pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin output ports from stereo
   * out */
  plugin_disconnect_from_prefader (pl, ch);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to channel
   * stereo out. connect it */
  plugin_connect_to_prefader (prev_pl, ch);
}

/**
 * Connect ports in the case of prev && next.
 */
NONNULL static void
disconnect_prev_next (Channel * ch, Plugin * prev_pl, Plugin * pl, Plugin * next_pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from
   * plugin */
  plugin_disconnect_from_plugin (prev_pl, pl);

  /* ------------------------------------------
   * Disconnect output ports
   * ------------------------------------------ */

  /* disconnect plugin's audio outs from next
   * plugin */
  plugin_disconnect_from_plugin (pl, next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to the next pl.
   * connect them */
  plugin_connect_to_plugin (prev_pl, next_pl);
}

void
Channel::prepare_process ()
{
  Plugin * plugin;
  PortType out_type = track_.out_signal_type;

  /* clear buffers */
  track_processor_clear_buffers (track_.processor);
  fader_clear_buffers (this->prefader);
  fader_clear_buffers (this->fader);

  if (out_type == PortType::Audio)
    {
      this->stereo_out->get_l ().clear_buffer (*AUDIO_ENGINE);
      this->stereo_out->get_r ().clear_buffer (*AUDIO_ENGINE);
    }
  else if (out_type == PortType::Event)
    {
      this->midi_out->clear_buffer (*AUDIO_ENGINE);
    }

  for (int j = 0; j < STRIP_SIZE; j++)
    {
      plugin = this->inserts[j];
      if (plugin)
        plugin_prepare_process (plugin);
      plugin = this->midi_fx[j];
      if (plugin)
        plugin_prepare_process (plugin);
    }
  if (this->instrument)
    plugin_prepare_process (this->instrument);

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_prepare_process (this->sends[i]);
    }

  if (track_.in_signal_type == PortType::Event)
    {
#ifdef HAVE_RTMIDI
      /* extract the midi events from the ring buffer */
      if (midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend))
        {
          track_.processor->midi_in->prepare_rtmidi_events ();
        }
#endif

      /* copy the cached MIDI events to the MIDI events in the MIDI in port */
      midi_events_dequeue (track_.processor->midi_in->midi_events_);
    }
}

void
Channel::init_loaded (Track &track)
{
  g_debug ("initing channel");

  this->track_ = track;
  this->magic = CHANNEL_MAGIC;

  /* fader */
  this->prefader->track = &track;
  this->fader->track = &track;

  fader_init_loaded (this->prefader, &track, NULL, NULL);
  fader_init_loaded (this->fader, &track, NULL, NULL);

  PortType out_type = track_.out_signal_type;

  switch (out_type)
    {
    case PortType::Event:
      this->midi_out->midi_events_ = midi_events_new ();
      break;
    case PortType::Audio:
      /* make sure master is exposed to backend */
      if (track_.type == TrackType::TRACK_TYPE_MASTER)
        {
          stereo_out->set_expose_to_backend (true);
        }
      break;
    default:
      break;
    }

  auto init_plugin =
    [this] (Plugin * _pl, int _slot, ZPluginSlotType _slot_type) {
      if (_pl)
        {
          _pl->id.track_name_hash = track_get_name_hash (this->track_);
          _pl->id.slot = _slot;
          _pl->id.slot_type = _slot_type;
          plugin_init_loaded (_pl, &this->track_, NULL);
        }
    };

  /* init plugins */
  Plugin * pl;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      pl = this->inserts[i];
      init_plugin (pl, i, ZPluginSlotType::Z_PLUGIN_SLOT_INSERT);
      pl = this->midi_fx[i];
      init_plugin (pl, i, ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX);
    }
  if (this->instrument)
    {
      pl = this->instrument;
      init_plugin (pl, -1, ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT);
    }

  /* init sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_init_loaded (this->sends[i], &this->track_);
    }
}

void
Channel::set_magic ()
{
  Plugin * plugins[120];
  int      num_plugins = get_plugins (plugins);

  for (int i = 0; i < num_plugins; i++)
    {
      Plugin * pl = plugins[i];
      pl->magic = PLUGIN_MAGIC;
    }
}

void
Channel::expose_ports_to_backend ()
{
  Track * tr = &track_;

  /* skip if auditioner */
  if (track_is_auditioner (tr))
    return;

  g_message ("%s: %s", __func__, tr->name);

  if (tr->in_signal_type == PortType::Audio)
    {
      tr->processor->stereo_in->set_expose_to_backend (true);
    }
  if (tr->in_signal_type == PortType::Event)
    {
      tr->processor->midi_in->set_expose_to_backend (true);
    }
  if (tr->out_signal_type == PortType::Audio)
    {
      this->stereo_out->set_expose_to_backend (true);
    }
  if (tr->out_signal_type == PortType::Event)
    {
      this->midi_out->set_expose_to_backend (true);
    }
}

void
Channel::reconnect_ext_input_ports ()
{
  Track * track = &track_;

  /* skip if auditioner track */
  if (track_is_auditioner (track))
    return;

  if (track->disconnecting)
    {
      g_critical (
        "attempted to reconnect ext input ports on "
        "track %s while it is disconnecting",
        track->name);
      return;
    }

  g_return_if_fail (is_in_active_project ());

  g_debug ("reconnecting ext inputs for %s", track->name);

  if (
    track->type == TrackType::TRACK_TYPE_INSTRUMENT
    || track->type == TrackType::TRACK_TYPE_MIDI
    || track->type == TrackType::TRACK_TYPE_CHORD)
    {
      Port * midi_in = track->processor->midi_in;

      /* if the project was loaded with another backend, the port might not be
       * exposed yet, so expose it */
      midi_in->set_expose_to_backend (true);

      /* disconnect all connections to hardware */
      midi_in->disconnect_hw_inputs ();

      if (this->all_midi_ins)
        {
          for (int i = 0; i < HW_IN_PROCESSOR->num_selected_midi_ports; i++)
            {
              char * port_id = HW_IN_PROCESSOR->selected_midi_ports[i];
              Port * source =
                hardware_processor_find_port (HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_message ("port for %s not found", port_id);
                  continue;
                }
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR, &source->id_, &midi_in->id_, 1.f,
                F_LOCKED, F_ENABLE);
            }
        }
      /* else if not all stereo ins selected */
      else
        {
          for (int i = 0; i < this->num_ext_midi_ins; i++)
            {
              char * port_id = ext_port_get_id (this->ext_midi_ins[i]);
              Port * source =
                hardware_processor_find_port (HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_message ("port for %s not found", port_id);
                  g_free (port_id);
                  continue;
                }
              g_free (port_id);
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR, &source->id_, &midi_in->id_, 1.f,
                F_LOCKED, F_ENABLE);
            }
        }
    }
  else if (track->type == TrackType::TRACK_TYPE_AUDIO)
    {
      /* if the project was loaded with another backend, the port might not be
       * exposed yet, so expose it */
      track->processor->stereo_in->set_expose_to_backend (true);

      /* disconnect all connections to hardware */
      track->processor->stereo_in->disconnect_hw_inputs ();

      Port &l = track->processor->stereo_in->get_l ();
      Port &r = track->processor->stereo_in->get_r ();
      if (this->all_stereo_l_ins)
        {
          for (int i = 0; i < HW_IN_PROCESSOR->num_selected_audio_ports; i++)
            {
              char * port_id = HW_IN_PROCESSOR->selected_audio_ports[i];
              Port * source =
                hardware_processor_find_port (HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_message ("port for %s not found", port_id);
                  continue;
                }
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR, &source->id_, &l.id_, 1.f, F_LOCKED,
                F_ENABLE);
            }
        }
      else
        {
          g_debug ("%d L HW ins", this->num_ext_stereo_l_ins);
          for (int i = 0; i < this->num_ext_stereo_l_ins; i++)
            {
              char * port_id = ext_port_get_id (this->ext_stereo_l_ins[i]);
              Port * source =
                hardware_processor_find_port (HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_warning ("port for %s not found", port_id);
                  g_free (port_id);
                  continue;
                }
              g_free (port_id);
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR, &source->id_, &l.id_, 1.f, F_LOCKED,
                F_ENABLE);
            }
        } /* endif all audio ins for L */

      if (this->all_stereo_r_ins)
        {
          for (int i = 0; i < HW_IN_PROCESSOR->num_selected_audio_ports; i++)
            {
              char * port_id = HW_IN_PROCESSOR->selected_audio_ports[i];
              Port * source =
                hardware_processor_find_port (HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_message ("port for %s not found", port_id);
                  continue;
                }
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR, &source->id_, &r.id_, 1.f, F_LOCKED,
                F_ENABLE);
            }
        }
      /* else if not all audio ins for R */
      else
        {
          g_debug ("%d R HW ins", this->num_ext_stereo_r_ins);
          for (int i = 0; i < this->num_ext_stereo_r_ins; i++)
            {
              char * port_id = ext_port_get_id (this->ext_stereo_r_ins[i]);
              Port * source =
                hardware_processor_find_port (HW_IN_PROCESSOR, port_id);
              if (!source)
                {
                  g_warning ("port for %s not found", port_id);
                  g_free (port_id);
                  continue;
                }
              g_free (port_id);
              port_connections_manager_ensure_connect (
                PORT_CONNECTIONS_MGR, &source->id_, &r.id_, 1.f, F_LOCKED,
                F_ENABLE);
            }
        } /* endif all audio ins for R */
    }

  router_recalc_graph (ROUTER, false);
}

/**
 * Adds to (or subtracts from) the pan.
 */
void
Channel::add_balance_control (void * _channel, float pan)
{
  Channel * channel = static_cast<Channel *> (_channel);

  channel->fader->balance->set_control_value (
    CLAMP (channel->fader->balance->control_ + pan, 0.f, 1.f), 0, 0);
}

/**
 * Sets fader to 0.0.
 */
void
Channel::reset_fader (bool fire_events)
{
  fader_set_amp_with_action (
    this->fader, this->fader->amp->control_, 1.0f, true);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_CHANNEL_FADER_VAL_CHANGED, this);
    }
}

/**
 * Gets whether mono compatibility is enabled.
 */
bool
Channel::get_mono_compat_enabled ()
{
  return fader_get_mono_compat_enabled (this->fader);
}

/**
 * Sets whether mono compatibility is enabled.
 */
void
Channel::set_mono_compat_enabled (bool enabled, bool fire_events)
{
  fader_set_mono_compat_enabled (this->fader, enabled, fire_events);
}

/**
 * Gets whether mono compatibility is enabled.
 */
bool
Channel::get_swap_phase ()
{
  return fader_get_swap_phase (this->fader);
}

/**
 * Sets whether mono compatibility is enabled.
 */
void
Channel::set_swap_phase (bool enabled, bool fire_events)
{
  fader_set_swap_phase (this->fader, enabled, fire_events);
}

static void
connect_plugins (Channel * self)
{
  g_return_if_fail (self->is_in_active_project ());

  /* loop through each slot in each of MIDI FX,
   * instrument, inserts */
  for (int i = 0; i < 3; i++)
    {
      ZPluginSlotType slot_type;
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = NULL;
          int      slot = j;
          if (i == 0)
            {
              slot_type = ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX;
              plugin = self->midi_fx[j];
            }
          else if (i == 1)
            {
              slot_type = ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT;
              plugin = self->instrument;
            }
          else
            {
              slot_type = ZPluginSlotType::Z_PLUGIN_SLOT_INSERT;
              plugin = self->inserts[j];
            }
          if (!plugin)
            continue;

          if (!plugin->instantiated && !plugin->instantiation_failed)
            {
              /* TODO handle error */
              plugin_instantiate (plugin, NULL);
            }

          Plugin ** prev_plugins = NULL;
          switch (slot_type)
            {
            case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
              prev_plugins = self->inserts;
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
              prev_plugins = self->midi_fx;
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
              prev_plugins = self->midi_fx;
              break;
            default:
              g_return_if_reached ();
            }
          Plugin ** next_plugins = NULL;
          switch (slot_type)
            {
            case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
              next_plugins = self->inserts;
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
              next_plugins = self->midi_fx;
              break;
            case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
              next_plugins = self->inserts;
              break;
            default:
              g_return_if_reached ();
            }

          Plugin * next_pl = NULL;
          for (
            int k =
              (slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT
                 ? 0
                 : slot + 1);
            k < STRIP_SIZE; k++)
            {
              next_pl = next_plugins[k];
              if (next_pl)
                break;
            }
          if (!next_pl && slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX)
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
          /* if instrument, find previous MIDI FX
           * (if any) */
          if (slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
            {
              for (int k = STRIP_SIZE - 1; k >= 0; k--)
                {
                  prev_pl = prev_plugins[k];
                  if (prev_pl)
                    break;
                }
            }
          /* else if not instrument, check previous
           * plugins before this slot */
          else
            {
              for (int k = slot - 1; k >= 0; k--)
                {
                  prev_pl = prev_plugins[k];
                  if (prev_pl)
                    break;
                }
            }

          /* if still not found and slot is insert,
           * check instrument or MIDI FX where
           * applicable */
          if (!prev_pl && slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSERT)
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

          if (
            prev_pl && !prev_pl->instantiated && !prev_pl->instantiation_failed)
            {
              plugin_instantiate (prev_pl, NULL);
            }
          if (
            next_pl && !next_pl->instantiated && !next_pl->instantiation_failed)
            {
              plugin_instantiate (next_pl, NULL);
            }

          if (!prev_pl && !next_pl)
            {
              connect_no_prev_no_next (self, plugin);
            }
          else if (!prev_pl && next_pl)
            {
              connect_no_prev_next (self, plugin, next_pl);
            }
          else if (prev_pl && !next_pl)
            {
              connect_prev_no_next (self, prev_pl, plugin);
            }
          else if (prev_pl && next_pl)
            {
              connect_prev_next (self, prev_pl, plugin, next_pl);
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
Channel::connect ()
{
  Track * tr = &track_;

  g_return_if_fail (track_is_in_active_project (tr) || track_is_auditioner (tr));

  g_debug ("connecting channel...");

  /* set default output */
  if (tr->type == TrackType::TRACK_TYPE_MASTER && !track_is_auditioner (tr))
    {
      this->output_name_hash = 0;
      this->has_output = 0;
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->stereo_out->get_l ().id_,
        &MONITOR_FADER->stereo_in->get_l ().id_, 1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->stereo_out->get_r ().id_,
        &MONITOR_FADER->stereo_in->get_r ().id_, 1.f, F_LOCKED, F_ENABLE);
    }

  if (tr->out_signal_type == PortType::Audio)
    {
      /* connect stereo in to stereo out through
       * fader */
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->prefader->stereo_out->get_l ().id_,
        &this->fader->stereo_in->get_l ().id_, 1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->prefader->stereo_out->get_r ().id_,
        &this->fader->stereo_in->get_r ().id_, 1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->fader->stereo_out->get_l ().id_,
        &this->stereo_out->get_l ().id_, 1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->fader->stereo_out->get_r ().id_,
        &this->stereo_out->get_r ().id_, 1.f, F_LOCKED, F_ENABLE);
    }
  else if (tr->out_signal_type == PortType::Event)
    {
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->prefader->midi_out->id_,
        &this->fader->midi_in->id_, 1.f, F_LOCKED, F_ENABLE);
      port_connections_manager_ensure_connect (
        PORT_CONNECTIONS_MGR, &this->fader->midi_out->id_, &this->midi_out->id_,
        1.f, F_LOCKED, F_ENABLE);
    }

  /** Connect MIDI in and piano roll to MIDI
   * out. */
  track_processor_connect_to_prefader (tr->processor);

  /* connect plugins */
  connect_plugins (this);

  /* expose ports to backend */
  if (AUDIO_ENGINE && AUDIO_ENGINE->setup)
    {
      expose_ports_to_backend ();
    }

  /* connect sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * send = this->sends[i];
      channel_send_connect_to_owner (send);
    }

  /* connect the designated midi inputs */
  reconnect_ext_input_ports ();

  g_message ("done connecting thisannel");
}

Track &
Channel::get_track ()
{
  return this->track_;
}

Track *
Channel::get_output_track ()
{
  if (!this->has_output)
    return NULL;

  Track * track = &(get_track ());
  g_return_val_if_fail (track, NULL);
  Tracklist * tracklist = track_get_tracklist (track);
  Track *     output_track =
    tracklist_find_track_by_name_hash (tracklist, this->output_name_hash);
  g_return_val_if_fail (output_track && track != output_track, NULL);

  return output_track;
}

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 */
void
Channel::append_ports (GPtrArray * ports, bool include_plugins)
{
  auto add_port = [ports] (Port * port) { g_ptr_array_add (ports, port); };

  /* add channel ports */
  if (track_.out_signal_type == PortType::Audio)
    {
      add_port (&this->stereo_out->get_l ());
      add_port (&this->stereo_out->get_r ());

      /* add fader ports */
      add_port (&this->fader->stereo_in->get_l ());
      add_port (&this->fader->stereo_in->get_r ());
      add_port (&this->fader->stereo_out->get_l ());
      add_port (&this->fader->stereo_out->get_r ());

      /* add prefader ports */
      add_port (&this->prefader->stereo_in->get_l ());
      add_port (&this->prefader->stereo_in->get_r ());
      add_port (&this->prefader->stereo_out->get_l ());
      add_port (&this->prefader->stereo_out->get_r ());
    }
  else if (track_.out_signal_type == PortType::Event)
    {
      add_port (this->midi_out);

      /* add fader ports */
      add_port (this->fader->midi_in);
      add_port (this->fader->midi_out);

      /* add prefader ports */
      add_port (this->prefader->midi_in);
      add_port (this->prefader->midi_out);
    }

  /* add fader amp and balance control */
  add_port (this->prefader->amp);
  add_port (this->prefader->balance);
  add_port (this->prefader->mute);
  add_port (this->prefader->solo);
  add_port (this->prefader->listen);
  add_port (this->prefader->mono_compat_enabled);
  add_port (this->prefader->swap_phase);
  add_port (this->fader->amp);
  add_port (this->fader->balance);
  add_port (this->fader->mute);
  add_port (this->fader->solo);
  add_port (this->fader->listen);
  add_port (this->fader->mono_compat_enabled);
  add_port (this->fader->swap_phase);

  if (include_plugins)
    {
      auto add_plugin_ports = [ports] (Plugin * pl) {
        if (pl)
          plugin_append_ports (pl, ports);
      };

      /* add plugin ports */
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          add_plugin_ports (this->inserts[j]);
          add_plugin_ports (this->midi_fx[j]);
        }

      add_plugin_ports (this->instrument);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel_send_append_ports (this->sends[i], ports);
    }
}

/**
 * Inits the stereo ports of the Channel while exposing them to the backend.
 *
 * This assumes the caller already checked that this channel should have the
 * given ports enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if new.
 */
static void
init_stereo_out_ports (Channel * self, bool loading)
{
  if (loading)
    {
      return;
    }

  Port l = Port (
    PortType::Audio, PortFlow::Output, "Stero out L",
    PortIdentifier::OwnerType::CHANNEL, self);
  l.id_.sym_ = "stereo_out_l";

  Port r = Port (
    PortType::Audio, PortFlow::Output, "Stereo out R",
    PortIdentifier::OwnerType::CHANNEL, self);
  r.id_.sym_ = "stereo_out_r";

  self->stereo_out = new StereoPorts (std::move (l), std::move (r));
}

Channel::Channel (Track &track) : track_ (track)
{
  track.channel = this;
  this->magic = CHANNEL_MAGIC;
  this->track_pos = track.pos;

  /* autoconnect to all midi ins and midi chans */
  this->all_midi_ins = 1;
  this->all_midi_channels = 1;

  /* create ports */
  switch (track_.out_signal_type)
    {
    case PortType::Audio:
      init_stereo_out_ports (this, 0);
      break;
    case PortType::Event:
      {
        Port * port = new Port (
          PortType::Event, PortFlow::Output, _ ("MIDI out"),
          PortIdentifier::OwnerType::CHANNEL, this);
        port->id_.sym_ = "midi_out";
        this->midi_out = port;
      }
      break;
    default:
      break;
    }

  FaderType fader_type = track_get_fader_type (&track);
  FaderType prefader_type = track_type_get_prefader_type (track_.type);
  this->fader = fader_new (fader_type, false, &track, NULL, NULL);
  this->prefader = fader_new (prefader_type, true, &track, NULL, NULL);

  /* init sends */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      this->sends[i] =
        channel_send_new (track_get_name_hash (track_), i, &track);
      this->sends[i]->track = &track;
    }
}

void
Channel::set_phase (void * _channel, float phase)
{
  Channel * channel = (Channel *) _channel;
  channel->fader->phase = phase;

  /* FIXME use an event */
  /*if (channel->widget)*/
  /*gtk_label_set_text (channel->widget->phase_reading,*/
  /*g_strdup_printf ("%.1f", phase));*/
}

float
Channel::get_phase (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader->phase;
}

void
Channel::set_balance_control (void * _channel, float val)
{
  Channel * ch = (Channel *) _channel;
  ch->set_balance_control (val);
}

void
Channel::set_balance_control (float val)
{
  fader->balance->set_control_value (val, 0, 0);
}

float
Channel::get_balance_control (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->get_balance_control ();
}

float
Channel::get_balance_control () const
{
  return fader->balance->get_control_value (false);
}

static inline void
disconnect_plugin_from_strip (Channel * ch, int pos, Plugin * pl)
{
  int             i;
  ZPluginSlotType slot_type = pl->id.slot_type;
  Plugin **       prev_plugins = NULL;
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      prev_plugins = ch->inserts;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      prev_plugins = ch->midi_fx;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      prev_plugins = ch->midi_fx;
      break;
    default:
      g_return_if_reached ();
    }
  Plugin ** next_plugins = NULL;
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      next_plugins = ch->inserts;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      next_plugins = ch->midi_fx;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
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
  if (!next_plugin && slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX)
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
  if (!prev_plugin && slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSERT)
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
      disconnect_prev_next (ch, prev_plugin, pl, next_plugin);
    }

  /* unexpose all JACK ports */
  plugin_expose_ports (pl, F_NOT_EXPOSE, true, true);
}

void
Channel::remove_plugin (
  ZPluginSlotType slot_type,
  int             slot,
  bool            moving_plugin,
  bool            deleting_plugin,
  bool            deleting_channel,
  bool            recalc_graph)
{
  Plugin * plugin = NULL;
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      plugin = this->inserts[slot];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      plugin = this->midi_fx[slot];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      plugin = this->instrument;
      break;
    default:
      break;
    }
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (plugin));
  g_return_if_fail (plugin->id.track_name_hash == track_get_name_hash (track_));

  g_message (
    "Removing %s from %s:%s:%d", plugin->setting->descr->name, track_.name,
    ENUM_NAME (slot_type), slot);

  /* if moving, the move is already handled in
   * plugin_move_automation() inside
   * plugin_move(). */
  if (!moving_plugin)
    {
      plugin_remove_ats_from_automation_tracklist (
        plugin, deleting_plugin, !deleting_channel && !deleting_plugin);
    }

  if (is_in_active_project ())
    disconnect_plugin_from_strip (this, slot, plugin);

  /* if deleting plugin disconnect the plugin
   * entirely */
  if (deleting_plugin)
    {
      if (is_in_active_project () && plugin_is_selected (plugin))
        {
          mixer_selections_remove_slot (
            MIXER_SELECTIONS, plugin->id.slot, slot_type, F_PUBLISH_EVENTS);
        }

      plugin_disconnect (plugin);
      object_free_w_func_and_null (plugin_free, plugin);
    }

  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      this->inserts[slot] = NULL;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      this->midi_fx[slot] = NULL;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      this->instrument = NULL;
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  if (
    track_is_in_active_project (&track_)
    && !track_.disconnecting
    /* only verify if we are deleting the plugin. if the plugin is moved to
       another slot this check fails because the port identifiers in the
       automation tracks are already updated to point to the next slot and the
       plugin is not found there yet */
    && deleting_plugin && !moving_plugin)
    {
      track_validate (&track_);
    }

  if (recalc_graph)
    router_recalc_graph (ROUTER, F_NOT_SOFT);
}

bool
Channel::add_plugin (
  ZPluginSlotType slot_type,
  int             slot,
  Plugin *        plugin,
  bool            confirm,
  bool            moving_plugin,
  bool            gen_automatables,
  bool            recalc_graph,
  bool            pub_events)
{
  g_return_val_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (slot_type, slot), false);

  bool prev_enabled = track_.enabled;
  track_.enabled = false;

  Plugin ** plugins = NULL;
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      plugins = this->inserts;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      plugins = this->midi_fx;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      break;
    default:
      g_return_val_if_reached (0);
    }

  Plugin * existing_pl = NULL;
  if (slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    existing_pl = this->instrument;
  else
    existing_pl = plugins[slot];

  if (existing_pl)
    {
      g_message ("existing plugin exists at %s:%d", track_.name, slot);
    }

  /* free current plugin */
  if (existing_pl)
    {
      remove_plugin (
        slot_type, slot, moving_plugin, F_DELETING_PLUGIN,
        F_NOT_DELETING_CHANNEL, F_NO_RECALC_GRAPH);
    }

  g_message (
    "Inserting %s %s at %s:%s:%d", ENUM_NAME (slot_type),
    plugin->setting->descr->name, track_.name, ENUM_NAME (slot_type), slot);
  if (slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      this->instrument = plugin;
    }
  else
    {
      plugins[slot] = plugin;
    }
  plugin->track = &track_;
  plugin_set_track_and_slot (
    plugin, track_get_name_hash (track_), slot_type, slot);
  g_return_val_if_fail (plugin->track, false);

  Plugin ** prev_plugins = NULL;
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      prev_plugins = this->inserts;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      prev_plugins = this->midi_fx;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      prev_plugins = this->midi_fx;
      break;
    default:
      g_return_val_if_reached (0);
    }
  Plugin ** next_plugins = NULL;
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      next_plugins = this->inserts;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      next_plugins = this->midi_fx;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      next_plugins = this->inserts;
      break;
    default:
      g_return_val_if_reached (0);
    }

  Plugin * next_pl = NULL;
  for (
    int i =
      (slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT ? 0 : slot + 1);
    i < STRIP_SIZE; i++)
    {
      next_pl = next_plugins[i];
      if (next_pl)
        break;
    }
  if (!next_pl && slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX)
    {
      if (this->instrument)
        next_pl = this->instrument;
      else
        {
          for (int i = 0; i < STRIP_SIZE; i++)
            {
              next_pl = this->inserts[i];
              if (next_pl)
                break;
            }
        }
    }

  Plugin * prev_pl = NULL;
  if (slot_type != ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      for (int i = slot - 1; i >= 0; i--)
        {
          prev_pl = prev_plugins[i];
          if (prev_pl)
            break;
        }
    }
  if (!prev_pl && slot_type == ZPluginSlotType::Z_PLUGIN_SLOT_INSERT)
    {
      if (this->instrument)
        prev_pl = this->instrument;
      else
        {
          for (int i = STRIP_SIZE - 1; i >= 0; i--)
            {
              prev_pl = this->midi_fx[i];
              if (prev_pl)
                break;
            }
        }
    }

  /* ------------------------------------------
   * connect ports
   * ------------------------------------------ */

  g_debug ("%s: connecting plugin ports...", __func__);

  if (track_is_in_active_project (&track_))
    {
      connect_plugins (this);
    }

  track_.enabled = prev_enabled;

  if (gen_automatables)
    {
      plugin_generate_automation_tracks (plugin, &track_);
    }

  if (pub_events)
    {
      EVENTS_PUSH (EventType::ET_PLUGIN_ADDED, plugin);
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
NONNULL void
Channel::update_track_name_hash (
  unsigned int old_name_hash,
  unsigned int new_name_hash)
{
  /* update output */
  if (track_is_in_active_project (&track_) && this->has_output)
    {
      Track * out_track = Channel::get_output_track ();
      g_return_if_fail (IS_TRACK_AND_NONNULL (out_track));
      int child_idx = group_target_track_find_child (out_track, old_name_hash);
      g_return_if_fail (child_idx >= 0);

      out_track->children[child_idx] = new_name_hash;
      g_debug (
        "%s: setting output of track '%s' to '%s'", __func__, this->track_.name,
        out_track->name);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * send = this->sends[i];
      send->track_name_hash = new_name_hash;
    }

#define SET_PLUGIN_NAME_HASH(pl) \
  if (pl) \
  plugin_set_track_name_hash (pl, new_name_hash)

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      SET_PLUGIN_NAME_HASH (this->inserts[i]);
      SET_PLUGIN_NAME_HASH (this->midi_fx[i]);
    }
  SET_PLUGIN_NAME_HASH (this->instrument);
}

AutomationTrack *
Channel::get_automation_track (PortIdentifier::Flags port_flags)
{
  AutomationTracklist * atl = track_get_automation_tracklist (&track_);
  g_return_val_if_fail (atl, NULL);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];

      if (
        ENUM_BITSET_TEST (PortIdentifier::Flags, at->port_id.flags_, port_flags))
        return at;
    }
  return nullptr;
}

Plugin *
Channel::get_plugin_at (int slot, ZPluginSlotType slot_type) const
{
  switch (slot_type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      return this->inserts[slot];
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      return this->midi_fx[slot];
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      return this->instrument;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
    default:
      g_return_val_if_reached (nullptr);
    }
}

typedef struct PluginImportData
{
  Channel *                ch;
  const Plugin *           pl;
  const MixerSelections *  sel;
  const PluginDescriptor * descr;
  int                      slot;
  ZPluginSlotType          slot_type;
  bool                     copy;
  bool                     ask_if_overwrite;
} PluginImportData;

static void
plugin_import_data_free (void * _data)
{
  PluginImportData * self = (PluginImportData *) _data;
  object_zero_and_free (self);
}

static void
do_import (PluginImportData * data)
{
  bool plugin_valid = true;
  if (data->pl)
    {
      Track * orig_track = tracklist_find_track_by_name_hash (
        TRACKLIST, data->pl->id.track_name_hash);

      /* if plugin at original position do nothing */
      if (
        &data->ch->track_ == orig_track && data->slot == data->pl->id.slot
        && data->slot_type == data->pl->id.slot_type)
        return;

      if (plugin_descriptor_is_valid_for_slot_type (
            data->pl->setting->descr, data->slot_type, data->ch->track_.type))
        {
          bool     ret;
          GError * err = NULL;
          if (data->copy)
            {
              ret = mixer_selections_action_perform_copy (
                data->sel, PORT_CONNECTIONS_MGR, data->slot_type,
                track_get_name_hash (data->ch->track_), data->slot, &err);
            }
          else
            {
              ret = mixer_selections_action_perform_move (
                data->sel, PORT_CONNECTIONS_MGR, data->slot_type,
                track_get_name_hash (data->ch->track_), data->slot, &err);
            }

          if (!ret)
            {
              HANDLE_ERROR_LITERAL (err, _ ("Failed to move or copy plugins"));
              return;
            }
        }
      else
        {
          plugin_valid = false;
        }
    }
  else if (data->descr)
    {
      /* validate */
      if (plugin_descriptor_is_valid_for_slot_type (
            data->descr, data->slot_type, data->ch->track_.type))
        {
          PluginSetting * setting = plugin_setting_new_default (data->descr);
          GError *        err = NULL;
          bool            ret = mixer_selections_action_perform_create (
            data->slot_type, track_get_name_hash (data->ch->track_), data->slot,
            setting, 1, &err);
          if (ret)
            {
              plugin_setting_increment_num_instantiations (setting);
            }
          else
            {
              HANDLE_ERROR (
                err, _ ("Failed to create plugin %s"), setting->descr->name);
              plugin_setting_free (setting);
              return;
            }
          plugin_setting_free (setting);
        }
      else
        {
          plugin_valid = false;
        }
    }
  else
    {
      g_critical ("No descriptor or plugin given");
      return;
    }

  if (!plugin_valid)
    {
      const PluginDescriptor * pl_descr =
        data->descr ? data->descr : data->pl->setting->descr;
      GError * err = NULL;
      g_set_error (
        &err, Z_DSP_CHANNEL_ERROR, Z_DSP_CHANNEL_ERROR_FAILED,
        _ ("Plugin %s cannot be added to this slot"), pl_descr->name);
      HANDLE_ERROR_LITERAL (err, _ ("Failed to add plugin"));
    }
}

static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  PluginImportData * data = (PluginImportData *) user_data;
  if (!string_is_equal (response, "overwrite"))
    {
      return;
    }

  do_import (data);
}

void
Channel::handle_plugin_import (
  const Plugin *           pl,
  const MixerSelections *  sel,
  const PluginDescriptor * descr,
  int                      slot,
  ZPluginSlotType          slot_type,
  bool                     copy,
  bool                     ask_if_overwrite)
{
  PluginImportData * data = object_new (PluginImportData);
  data->ch = this;
  data->sel = sel;
  data->descr = descr;
  data->slot = slot;
  data->slot_type = slot_type;
  data->copy = copy;
  data->pl = pl && sel && sel->has_any ? sel->plugins[0] : NULL;

  g_message ("handling plugin import on channel %s...", this->track_.name);

  if (ask_if_overwrite)
    {
      bool show_dialog = false;
      if (pl)
        {
          for (int i = 0; i < sel->num_slots; i++)
            {
              if (get_plugin_at (slot + i, slot_type))
                {
                  show_dialog = true;
                  break;
                }
            }
        }
      else
        {
          if (get_plugin_at (slot, slot_type))
            {
              show_dialog = true;
            }
        }

      if (show_dialog)
        {
          AdwMessageDialog * dialog =
            dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
          gtk_window_present (GTK_WINDOW (dialog));
          g_signal_connect_data (
            dialog, "response", G_CALLBACK (overwrite_plugin_response_cb), data,
            (GClosureNotify) plugin_import_data_free, G_CONNECT_DEFAULT);
          return;
        }
    }

  do_import (data);
  plugin_import_data_free (data);
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
Channel::clone (Track &track, GError ** error)
{
  g_return_val_if_fail (!error || !*error, NULL);

  Channel * clone = new Channel (track);

  clone->fader->track = &clone->track_;
  clone->prefader->track = &clone->track_;
  fader_copy_values (this->fader, clone->fader);

  clone->has_output = this->has_output;
  clone->output_name_hash = this->output_name_hash;

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * src_send = this->sends[i];
      channel_send_copy_values (clone->sends[i], src_send);
    }

#define CLONE_AND_ADD_PL(pl, slot_type, slot) \
  { \
    GError * err = NULL; \
    Plugin * clone_pl = plugin_clone (pl, &err); \
    if (!clone_pl) \
      { \
        PROPAGATE_PREFIXED_ERROR ( \
          error, err, "%s", _ ("Failed to clone plugin")); \
        object_delete_and_null (clone); \
        return nullptr; \
      } \
    clone->add_plugin ( \
      slot_type, slot, clone_pl, F_NO_CONFIRM, F_NOT_MOVING_PLUGIN, \
      F_GEN_AUTOMATABLES, F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS); \
  }

  /* copy plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (this->inserts[i])
        {
          CLONE_AND_ADD_PL (
            this->inserts[i], ZPluginSlotType::Z_PLUGIN_SLOT_INSERT, i);
        }
    }
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (this->midi_fx[i])
        {
          CLONE_AND_ADD_PL (
            this->midi_fx[i], ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX, i);
        }
    }
  if (this->instrument)
    {
      CLONE_AND_ADD_PL (
        this->instrument, ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT, -1);
    }

#undef CLONE_AND_ADD_PL

  /* copy port connection info (including
   * plugins) */
  GPtrArray * ports = g_ptr_array_new ();
  append_ports (ports, true);
  GPtrArray * ports_clone = g_ptr_array_new ();
  append_ports (ports_clone, true);
  g_return_val_if_fail (ports->len == ports_clone->len, NULL);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      Port * clone_port = (Port *) g_ptr_array_index (ports_clone, i);
      g_return_val_if_fail (port->id_.is_equal (clone_port->id_), NULL);
      clone_port->copy_values (*port);
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
Channel::select_all (ZPluginSlotType type, bool select)
{
  mixer_selections_clear (MIXER_SELECTIONS, F_PUBLISH_EVENTS);
  if (!select)
    return;

  switch (type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          if (this->inserts[i])
            {
              plugin_select (this->inserts[i], F_SELECT, F_NOT_EXCLUSIVE);
            }
        }
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          if (this->midi_fx[i])
            {
              plugin_select (this->midi_fx[i], F_SELECT, F_NOT_EXCLUSIVE);
            }
        }
      break;
    default:
      g_warning ("not implemented");
      break;
    }
}

/**
 * Sets caches for processing.
 */
void
Channel::set_caches ()
{
  Plugin * pls[120];
  int      num_pls = Channel::get_plugins (pls);

  for (int i = 0; i < num_pls; i++)
    {
      Plugin * pl = pls[i];
      plugin_set_caches (pl);
    }
}

int
Channel::get_plugins (Plugin ** pls)
{
  int size = 0;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (this->inserts[i])
        {
          pls[size++] = this->inserts[i];
        }
    }
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (this->midi_fx[i])
        {
          pls[size++] = this->midi_fx[i];
        }
    }
  if (this->instrument)
    {
      pls[size++] = this->instrument;
    }

  return size;
}

void
Channel::disconnect (bool remove_pl)
{
  g_debug ("disconnecting channel %s", track_.name);
  if (remove_pl)
    {
      FOREACH_STRIP
      {
        if (this->inserts[i])
          {
            Channel::remove_plugin (
              ZPluginSlotType::Z_PLUGIN_SLOT_INSERT, i, F_NOT_MOVING_PLUGIN,
              remove_pl, false, F_NO_RECALC_GRAPH);
          }
        if (this->midi_fx[i])
          {
            Channel::remove_plugin (
              ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX, i, F_NOT_MOVING_PLUGIN,
              remove_pl, false, F_NO_RECALC_GRAPH);
          }
      }
      if (this->instrument)
        {
          Channel::remove_plugin (
            ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT, 0, F_NOT_MOVING_PLUGIN,
            remove_pl, false, F_NO_RECALC_GRAPH);
        }
    }

  /* disconnect from output */
  if (Channel::is_in_active_project () && this->has_output)
    {
      Track * out_track = Channel::get_output_track ();
      g_return_if_fail (IS_TRACK_AND_NONNULL (out_track));
      group_target_track_remove_child (
        out_track, track_get_name_hash (track_), F_DISCONNECT,
        F_NO_RECALC_GRAPH, F_NO_PUBLISH_EVENTS);
    }

  /* disconnect fader/prefader */
  fader_disconnect_all (this->prefader);
  fader_disconnect_all (this->fader);

  /* disconnect all ports */
  GPtrArray * ports = g_ptr_array_new ();
  Channel::append_ports (ports, true);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      if (
        !IS_PORT (port)
        || port->is_in_active_project () != track_is_in_active_project (&track_))
        {
          g_critical ("invalid port");
          g_ptr_array_unref (ports);
          return;
        }

      port->disconnect_all ();
    }
  g_ptr_array_unref (ports);
}

Channel::~Channel ()
{
  object_free_w_func_and_null (fader_free, this->prefader);
  object_free_w_func_and_null (fader_free, this->fader);

  object_delete_and_null (this->stereo_out);
  object_delete_and_null (this->midi_out);

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      ChannelSend * send = this->sends[i];
      object_free_w_func_and_null (channel_send_free, send);
    }

  if (Z_IS_CHANNEL_WIDGET (this->widget))
    {
      this->widget->channel = NULL;
    }
}
