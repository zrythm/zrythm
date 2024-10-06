// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <ranges>

#include "common/dsp/automation_track.h"
#include "common/dsp/automation_tracklist.h"
#include "common/dsp/channel.h"
#include "common/dsp/ext_port.h"
#include "common/dsp/group_target_track.h"
#include "common/dsp/hardware_processor.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/port.h"
#include "common/dsp/port_connections_manager.h"
#include "common/dsp/position.h"
#include "common/dsp/router.h"
#include "common/dsp/rtmidi_device.h"
#include "common/dsp/track.h"
#include "common/dsp/track_processor.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/plugin.h"
#include "common/plugins/plugin_identifier.h"
#include "common/utils/dialogs.h"
#include "common/utils/logger.h"
#include "common/utils/objects.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/channel.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/track.h"
#include "gui/backend/gtk_widgets/tracklist.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

Channel::Channel (ChannelTrack &track)
    : track_pos_ (track.pos_), track_ (&track)
{
}

void
Channel::init_after_cloning (const Channel &other)
{
  track_pos_ = other.track_pos_;
  clone_variant_container<PluginVariant> (midi_fx_, other.midi_fx_);
  clone_variant_container<PluginVariant> (inserts_, other.inserts_);
  if (other.instrument_)
    instrument_ =
      clone_unique_with_variant<PluginVariant> (other.instrument_.get ());
  clone_unique_ptr_container (sends_, other.sends_);
  clone_unique_ptr_container (ext_midi_ins_, other.ext_midi_ins_);
  all_midi_ins_ = other.all_midi_ins_;
  clone_unique_ptr_container (ext_stereo_l_ins_, other.ext_stereo_l_ins_);
  all_stereo_l_ins_ = other.all_stereo_l_ins_;
  clone_unique_ptr_container (ext_stereo_r_ins_, other.ext_stereo_r_ins_);
  all_stereo_r_ins_ = other.all_stereo_r_ins_;
  midi_channels_ = other.midi_channels_;
  all_midi_channels_ = other.all_midi_channels_;
  fader_ = other.fader_->clone_unique ();
  prefader_ = other.prefader_->clone_unique ();
  if (other.midi_out_)
    midi_out_ = other.midi_out_->clone_unique ();
  if (other.stereo_out_)
    stereo_out_ = other.stereo_out_->clone_unique ();
  has_output_ = other.has_output_;
  output_name_hash_ = other.output_name_hash_;
  width_ = other.width_;
  track_ = nullptr; // clear previous track
}

void
Channel::set_track_ptr (ChannelTrack &track)
{
  track_ = &track;

  prefader_->track_ = track_;
  fader_->track_ = track_;

  std::vector<Plugin *> pls;
  get_plugins (pls);
  for (auto * pl : pls)
    {
      pl->track_ = track_;
    }

  for (auto &send : sends_)
    {
      send->track_ = track_;
    }
}

bool
Channel::is_in_active_project () const
{
  return track_->is_in_active_project ();
}

void
Channel::connect_no_prev_no_next (Plugin &pl)
{
  z_debug ("connect no prev no next");

  auto * track = get_track ();

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to channel stereo out. disconnect it */
  track->processor_->disconnect_from_prefader ();

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track->processor_->connect_to_plugin (pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin to stereo out */
  pl.connect_to_prefader (*this);
}

void
Channel::connect_no_prev_next (Plugin &pl, Plugin &next_pl)
{
  z_debug ("connect no prev next");

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* channel stereo in is connected to next plugin. disconnect it */
  track_->processor_->disconnect_from_plugin (next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect channel stereo in to plugin */
  track_->processor_->connect_to_plugin (pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin's audio outs to next plugin */
  pl.connect_to_plugin (next_pl);
}

void
Channel::connect_prev_no_next (Plugin &prev_pl, Plugin &pl)
{
  z_debug ("connect prev no next");

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to channel stereo out. disconnect it */
  prev_pl.disconnect_from_prefader (*this);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's outs to plugin */
  prev_pl.connect_to_plugin (pl);

  /* --------------------------------------
   * connect output ports
   * ------------------------------------*/

  /* connect plugin output ports to stereo_out */
  pl.connect_to_prefader (*this);
}

void
Channel::connect_prev_next (Plugin &prev_pl, Plugin &pl, Plugin &next_pl)
{
  z_debug ("connect prev next");

  /* -----------------------------------------
   * disconnect ports
   * ----------------------------------------- */
  /* prev plugin is connected to the next pl. disconnect them */
  prev_pl.disconnect_from_plugin (next_pl);

  /* -------------------------------------------
   * connect input ports
   * ------------------------------------------- */

  /* connect previous plugin's audio outs to plugin */
  prev_pl.connect_to_plugin (pl);

  /* ------------------------------------------
   * Connect output ports
   * ------------------------------------------ */

  /* connect plugin's audio outs to next plugin */
  pl.connect_to_plugin (next_pl);
}

void
Channel::disconnect_no_prev_no_next (Plugin &pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_->processor_->disconnect_from_plugin (pl);

  /* --------------------------------------
   * disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin from stereo out */
  pl.disconnect_from_prefader (*this);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo in should be connected to channel stereo out. connect it */
  track_->processor_->connect_to_prefader ();
}

void
Channel::disconnect_no_prev_next (Plugin &pl, Plugin &next_pl)
{
  /* -------------------------------------------
   * Disconnect input ports
   * ------------------------------------------- */

  /* disconnect channel stereo in from plugin */
  track_->processor_->disconnect_from_plugin (pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin's midi & audio outs from next plugin */
  pl.disconnect_from_plugin (next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* channel stereo out should be connected to next plugin. connect it */
  track_->processor_->connect_to_plugin (next_pl);
}

void
Channel::disconnect_prev_no_next (Plugin &prev_pl, Plugin &pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from plugin */
  prev_pl.disconnect_from_plugin (pl);

  /* --------------------------------------
   * Disconnect output ports
   * ------------------------------------*/

  /* disconnect plugin output ports from stereo out */
  pl.disconnect_from_prefader (*this);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to channel stereo out. connect it */
  prev_pl.connect_to_prefader (*this);
}

void
Channel::disconnect_prev_next (Plugin &prev_pl, Plugin &pl, Plugin &next_pl)
{
  /* -------------------------------------------
   * disconnect input ports
   * ------------------------------------------- */

  /* disconnect previous plugin's audio outs from plugin */
  prev_pl.disconnect_from_plugin (pl);

  /* ------------------------------------------
   * Disconnect output ports
   * ------------------------------------------ */

  /* disconnect plugin's audio outs from next plugin */
  pl.disconnect_from_plugin (next_pl);

  /* -----------------------------------------
   * connect ports
   * ----------------------------------------- */
  /* prev plugin should be connected to the next pl. connect them */
  prev_pl.connect_to_plugin (next_pl);
}

void
Channel::prepare_process (nframes_t nframes)
{
  PortType out_type = track_->out_signal_type_;

  /* clear buffers */
  track_->processor_->clear_buffers ();
  prefader_->clear_buffers ();
  fader_->clear_buffers ();

  if (out_type == PortType::Audio)
    {
      stereo_out_->get_l ().clear_buffer (*AUDIO_ENGINE);
      stereo_out_->get_r ().clear_buffer (*AUDIO_ENGINE);
    }
  else if (out_type == PortType::Event)
    {
      midi_out_->clear_buffer (*AUDIO_ENGINE);
    }

  for (auto &pl : inserts_)
    {
      if (pl)
        pl->prepare_process ();
    }
  for (auto &pl : midi_fx_)
    {
      if (pl)
        pl->prepare_process ();
    }
  if (instrument_)
    instrument_->prepare_process ();

  for (auto &send : sends_)
    {
      send->prepare_process ();
    }

  if (track_->in_signal_type_ == PortType::Event)
    {
#if HAVE_RTMIDI
      /* extract the midi events from the ring buffer */
      if (midi_backend_is_rtmidi (AUDIO_ENGINE->midi_backend_))
        {
          track_->processor_->midi_in_->prepare_rtmidi_events ();
        }
#endif

      /* copy the cached MIDI events to the MIDI events in the MIDI in port */
      track_->processor_->midi_in_->midi_events_.dequeue (0, nframes);
    }
}

void
Channel::init_loaded (ChannelTrack &track)
{
  z_debug ("initing channel");

  track_ = &track;

  /* fader */
  prefader_->track_ = &track;
  fader_->track_ = &track;

  prefader_->init_loaded (&track, nullptr, nullptr);
  fader_->init_loaded (&track, nullptr, nullptr);

  PortType out_type = track_->out_signal_type_;

  switch (out_type)
    {
    case PortType::Event:
      // this->midi_out->midi_events_ = midi_events_new ();
      break;
    case PortType::Audio:
      /* make sure master is exposed to backend */
      if (track_->is_master ())
        {
          stereo_out_->set_expose_to_backend (true);
        }
      break;
    default:
      break;
    }

  auto init_plugin = [this] (auto &_pl, int _slot, PluginSlotType _slot_type) {
    if (_pl)
      {
        auto &id = _pl->id_;
        id.track_name_hash_ = track_->get_name_hash ();
        id.slot_ = _slot;
        id.slot_type_ = _slot_type;
        _pl->init_loaded (track_, nullptr);
      }
  };

  /* init plugins */
  for (int i = 0; i < (int) STRIP_SIZE; i++)
    {
      init_plugin (inserts_.at (i), i, PluginSlotType::Insert);
      init_plugin (midi_fx_.at (i), i, PluginSlotType::MidiFx);
    }
  init_plugin (instrument_, -1, PluginSlotType::Instrument);

  /* init sends */
  for (auto &send : sends_)
    {
      send->init_loaded (track_);
    }
}

void
Channel::expose_ports_to_backend ()
{
  /* skip if auditioner */
  if (track_->is_auditioner ())
    return;

  z_debug ("exposing ports to backend: {}", track_->get_name ());
  if (track_->in_signal_type_ == PortType::Audio)
    {
      track_->processor_->stereo_in_->set_expose_to_backend (true);
    }
  if (track_->in_signal_type_ == PortType::Event)
    {
      track_->processor_->midi_in_->set_expose_to_backend (true);
    }
  if (track_->out_signal_type_ == PortType::Audio)
    {
      stereo_out_->set_expose_to_backend (true);
    }
  if (track_->out_signal_type_ == PortType::Event)
    {
      midi_out_->set_expose_to_backend (true);
    }
}

void
Channel::reconnect_ext_input_ports ()
{
  /* skip if auditioner track */
  if (track_->is_auditioner ())
    return;

  if (track_->disconnecting_)
    {
      z_error (
        "attempted to reconnect ext input ports on "
        "track {} while it is disconnecting",
        track_->name_);
      return;
    }

  z_return_if_fail (is_in_active_project ());

  z_debug ("reconnecting ext inputs for {}", track_->name_);

  if (track_->has_piano_roll () || track_->is_chord ())
    {
      auto &midi_in = track_->processor_->midi_in_;

      /* if the project was loaded with another backend, the port might not be
       * exposed yet, so expose it */
      midi_in->set_expose_to_backend (true);

      /* disconnect all connections to hardware */
      midi_in->disconnect_hw_inputs ();

      if (all_midi_ins_)
        {
          for (const auto &port_id : HW_IN_PROCESSOR->selected_midi_ports_)
            {
              auto source = HW_IN_PROCESSOR->find_port (port_id);
              if (!source)
                {
                  z_warning ("port for {} not found", port_id);
                  continue;
                }
              PORT_CONNECTIONS_MGR->ensure_connect (
                source->id_, midi_in->id_, 1.f, true, true);
            }
        }
      else
        {
          for (const auto &ext_port : ext_midi_ins_)
            {
              auto * source = HW_IN_PROCESSOR->find_port (ext_port->get_id ());
              if (source == nullptr)
                {
                  z_warning ("port for {} not found", ext_port->get_id ());
                  continue;
                }
              PORT_CONNECTIONS_MGR->ensure_connect (
                source->id_, midi_in->id_, 1.f, true, true);
            }
        }
    }
  else if (track_->type_ == Track::Type::Audio)
    {
      /* if the project was loaded with another backend, the port might not be
       * exposed yet, so expose it */
      track_->processor_->stereo_in_->set_expose_to_backend (true);

      /* disconnect all connections to hardware */
      track_->processor_->stereo_in_->disconnect_hw_inputs ();

      auto &l = track_->processor_->stereo_in_->get_l ();
      auto &r = track_->processor_->stereo_in_->get_r ();
      if (all_stereo_l_ins_)
        {
          for (const auto &port_id : HW_IN_PROCESSOR->selected_audio_ports_)
            {
              auto * source = HW_IN_PROCESSOR->find_port (port_id);
              if (source == nullptr)
                {
                  z_warning ("port for {} not found", port_id);
                  continue;
                }
              PORT_CONNECTIONS_MGR->ensure_connect (
                source->id_, l.id_, 1.f, true, true);
            }
        }
      else
        {
          z_debug ("{} L HW ins", ext_stereo_l_ins_.size ());
          for (const auto &ext_port : ext_stereo_l_ins_)
            {
              auto * source = HW_IN_PROCESSOR->find_port (ext_port->get_id ());
              if (source == nullptr)
                {
                  z_warning ("port for {} not found", ext_port->get_id ());
                  continue;
                }
              PORT_CONNECTIONS_MGR->ensure_connect (
                source->id_, l.id_, 1.f, true, true);
            }
        }

      if (all_stereo_r_ins_)
        {
          for (const auto &port_id : HW_IN_PROCESSOR->selected_audio_ports_)
            {
              auto * source = HW_IN_PROCESSOR->find_port (port_id);
              if (source == nullptr)
                {
                  z_warning ("port for {} not found", port_id);
                  continue;
                }
              PORT_CONNECTIONS_MGR->ensure_connect (
                source->id_, r.id_, 1.f, true, true);
            }
        }
      else
        {
          z_debug ("{} R HW ins", ext_stereo_r_ins_.size ());
          for (const auto &ext_port : ext_stereo_r_ins_)
            {
              auto * source = HW_IN_PROCESSOR->find_port (ext_port->get_id ());
              if (source == nullptr)
                {
                  z_warning ("port for {} not found", ext_port->get_id ());
                  continue;
                }
              PORT_CONNECTIONS_MGR->ensure_connect (
                source->id_, r.id_, 1.f, true, true);
            }
        }
    }

  ROUTER->recalc_graph (false);
}

void
Channel::add_balance_control (float pan)
{
  fader_->balance_->set_control_value (
    std::clamp<float> (fader_->balance_->control_ + pan, 0.f, 1.f), false,
    false);
}

void
Channel::reset_fader (bool fire_events)
{
  fader_->set_amp_with_action (fader_->amp_->control_, 1.0f, true);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_CHANNEL_FADER_VAL_CHANGED, this);
    }
}

bool
Channel::get_mono_compat_enabled ()
{
  return fader_->get_mono_compat_enabled ();
}

void
Channel::set_mono_compat_enabled (bool enabled, bool fire_events)
{
  fader_->set_mono_compat_enabled (enabled, fire_events);
}

bool
Channel::get_swap_phase ()
{
  return fader_->get_swap_phase ();
}

void
Channel::set_swap_phase (bool enabled, bool fire_events)
{
  fader_->set_swap_phase (enabled, fire_events);
}

void
Channel::connect_plugins ()
{
  z_return_if_fail (is_in_active_project ());

  /* loop through each slot in each of MIDI FX, instrument, inserts */
  for (int i = 0; i < 3; i++)
    {
      PluginSlotType slot_type;
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = nullptr;
          int      slot = j;
          if (i == 0)
            {
              slot_type = PluginSlotType::MidiFx;
              plugin = midi_fx_[j].get ();
            }
          else if (i == 1)
            {
              slot_type = PluginSlotType::Instrument;
              plugin = instrument_.get ();
            }
          else
            {
              slot_type = PluginSlotType::Insert;
              plugin = inserts_.at (j).get ();
            }
          if (plugin == nullptr)
            continue;

          if (!plugin->instantiated_ && !plugin->instantiation_failed_)
            {
              /* TODO handle error */
              plugin->instantiate ();
            }

          auto add_plugin_ptrs = [] (auto &plugins, std::vector<Plugin *> &dest) {
            for (auto &p : plugins)
              {
                dest.push_back (p.get ());
              }
          };

          std::vector<Plugin *> prev_plugins;
          switch (slot_type)
            {
            case PluginSlotType::Insert:
              add_plugin_ptrs (inserts_, prev_plugins);
              break;
            case PluginSlotType::Instrument:
            case PluginSlotType::MidiFx:
              add_plugin_ptrs (midi_fx_, prev_plugins);
              break;
            default:
              z_return_if_reached ();
            }
          std::vector<Plugin *> next_plugins;
          switch (slot_type)
            {
            case PluginSlotType::Insert:
              add_plugin_ptrs (inserts_, next_plugins);
              break;
            case PluginSlotType::MidiFx:
            case PluginSlotType::Instrument:
              add_plugin_ptrs (midi_fx_, next_plugins);
              break;
            default:
              z_return_if_reached ();
            }

          z_return_if_fail (next_plugins.size () == STRIP_SIZE);
          Plugin * next_pl = nullptr;
          for (
            size_t k = (slot_type == PluginSlotType::Instrument ? 0 : slot + 1);
            k < STRIP_SIZE; k++)
            {
              next_pl = next_plugins[k];
              if (next_pl != nullptr)
                break;
            }
          if ((next_pl == nullptr) && slot_type == PluginSlotType::MidiFx)
            {
              if (instrument_)
                next_pl = instrument_.get ();
              else
                {
                  for (auto &insert : inserts_)
                    {
                      next_pl = insert.get ();
                      if (next_pl != nullptr)
                        break;
                    }
                }
            }

          Plugin * prev_pl = nullptr;
          /* if instrument, find previous MIDI FX (if any) */
          if (slot_type == PluginSlotType::Instrument)
            {
              for (auto &prev_plugin : std::ranges::reverse_view (prev_plugins))
                {
                  if (prev_plugin)
                    {
                      prev_pl = prev_plugin;
                      break;
                    }
                }
            }
          /* else if not instrument, check previous plugins before this slot */
          else
            {
              for (int k = slot - 1; k >= 0; k--)
                {
                  prev_pl = prev_plugins[k];
                  if (prev_pl)
                    break;
                }
            }

          /* if still not found and slot is insert, check instrument or MIDI FX
           * where applicable */
          if (!prev_pl && slot_type == PluginSlotType::Insert)
            {
              if (instrument_)
                prev_pl = instrument_.get ();
              else
                {
                  for (
                    auto it = midi_fx_.rbegin (); it != midi_fx_.rend (); ++it)
                    {
                      if (*it)
                        {
                          prev_pl = it->get ();
                          break;
                        }
                    }
                }
            }

          auto instantiate_if_needed = [] (Plugin * pl) {
            if (pl && !pl->instantiated_ && !pl->instantiation_failed_)
              {
                pl->instantiate ();
              }
          };

          try
            {
              instantiate_if_needed (prev_pl);
              instantiate_if_needed (next_pl);
            }
          catch (const ZrythmException &e)
            {
              z_warning ("plugin instantiation failed: {}", e.what ());
            }

          if (!prev_pl && !next_pl)
            {
              connect_no_prev_no_next (*plugin);
            }
          else if (!prev_pl && next_pl)
            {
              connect_no_prev_next (*plugin, *next_pl);
            }
          else if (prev_pl && !next_pl)
            {
              connect_prev_no_next (*prev_pl, *plugin);
            }
          else if (prev_pl && next_pl)
            {
              connect_prev_next (*prev_pl, *plugin, *next_pl);
            }

        } /* end for each slot */

    } /* end for each slot type */
}

void
Channel::connect ()
{
  auto &tr = track_;

  z_return_if_fail (tr->is_in_active_project () || tr->is_auditioner ());

  z_debug ("connecting channel...");

  /* set default output */
  if (tr->is_master () && !tr->is_auditioner ())
    {
      output_name_hash_ = 0;
      has_output_ = false;
      PORT_CONNECTIONS_MGR->ensure_connect (
        this->stereo_out_->get_l ().id_,
        MONITOR_FADER->stereo_in_->get_l ().id_, 1.f, true, true);
      PORT_CONNECTIONS_MGR->ensure_connect (
        this->stereo_out_->get_r ().id_,
        MONITOR_FADER->stereo_in_->get_r ().id_, 1.f, true, true);
    }

  if (tr->out_signal_type_ == PortType::Audio)
    {
      /* connect stereo in to stereo out through fader */
      PORT_CONNECTIONS_MGR->ensure_connect (
        prefader_->stereo_out_->get_l ().id_, fader_->stereo_in_->get_l ().id_,
        1.f, true, true);
      PORT_CONNECTIONS_MGR->ensure_connect (
        prefader_->stereo_out_->get_r ().id_, fader_->stereo_in_->get_r ().id_,
        1.f, true, true);
      PORT_CONNECTIONS_MGR->ensure_connect (
        fader_->stereo_out_->get_l ().id_, stereo_out_->get_l ().id_, 1.f, true,
        true);
      PORT_CONNECTIONS_MGR->ensure_connect (
        fader_->stereo_out_->get_r ().id_, stereo_out_->get_r ().id_, 1.f, true,
        true);
    }
  else if (tr->out_signal_type_ == PortType::Event)
    {
      PORT_CONNECTIONS_MGR->ensure_connect (
        prefader_->midi_out_->id_, fader_->midi_in_->id_, 1.f, true, true);
      PORT_CONNECTIONS_MGR->ensure_connect (
        fader_->midi_out_->id_, midi_out_->id_, 1.f, true, true);
    }

  /** Connect MIDI in and piano roll to MIDI out. */
  tr->processor_->connect_to_prefader ();

  /* connect plugins */
  connect_plugins ();

  /* expose ports to backend */
  if (AUDIO_ENGINE && AUDIO_ENGINE->setup_)
    {
      expose_ports_to_backend ();
    }

  /* connect sends */
  for (auto &send : sends_)
    {
      send->track_ = track_;
      send->connect_to_owner ();
    }

  /* connect the designated midi inputs */
  reconnect_ext_input_ports ();

  z_info ("done connecting channel {}", track_->name_);
}

GroupTargetTrack *
Channel::get_output_track () const
{
  if (!has_output_)
    return nullptr;

  z_return_val_if_fail (track_, nullptr);
  auto tracklist = track_->get_tracklist ();
  auto output_track =
    tracklist->find_track_by_name_hash<GroupTargetTrack> (output_name_hash_);
  z_return_val_if_fail (output_track && track_ != output_track, nullptr);

  return output_track;
}

void
Channel::append_ports (std::vector<Port *> &ports, bool include_plugins)
{
  z_return_if_fail (track_);

  auto add_port = [&ports] (Port * port) { ports.push_back (port); };

  /* add channel ports */
  if (track_->out_signal_type_ == PortType::Audio)
    {
      add_port (&stereo_out_->get_l ());
      add_port (&stereo_out_->get_r ());

      /* add fader ports */
      add_port (&fader_->stereo_in_->get_l ());
      add_port (&fader_->stereo_in_->get_r ());
      add_port (&fader_->stereo_out_->get_l ());
      add_port (&fader_->stereo_out_->get_r ());

      /* add prefader ports */
      add_port (&prefader_->stereo_in_->get_l ());
      add_port (&prefader_->stereo_in_->get_r ());
      add_port (&prefader_->stereo_out_->get_l ());
      add_port (&prefader_->stereo_out_->get_r ());
    }
  else if (track_->out_signal_type_ == PortType::Event)
    {
      add_port (midi_out_.get ());

      /* add fader ports */
      add_port (fader_->midi_in_.get ());
      add_port (fader_->midi_out_.get ());

      /* add prefader ports */
      add_port (prefader_->midi_in_.get ());
      add_port (prefader_->midi_out_.get ());
    }

  /* add fader amp and balance control */
  add_port (prefader_->amp_.get ());
  add_port (prefader_->balance_.get ());
  add_port (prefader_->mute_.get ());
  add_port (prefader_->solo_.get ());
  add_port (prefader_->listen_.get ());
  add_port (prefader_->mono_compat_enabled_.get ());
  add_port (prefader_->swap_phase_.get ());
  add_port (fader_->amp_.get ());
  add_port (fader_->balance_.get ());
  add_port (fader_->mute_.get ());
  add_port (fader_->solo_.get ());
  add_port (fader_->listen_.get ());
  add_port (fader_->mono_compat_enabled_.get ());
  add_port (fader_->swap_phase_.get ());

  if (include_plugins)
    {
      auto add_plugin_ports = [&ports] (const auto &pl) {
        if (pl)
          pl->append_ports (ports);
      };

      /* add plugin ports */
      for (const auto &insert : inserts_)
        add_plugin_ports (insert);
      for (const auto &fx : midi_fx_)
        add_plugin_ports (fx);

      add_plugin_ports (instrument_);
    }

  for (const auto &send : sends_)
    {
      send->append_ports (ports);
    }
}

void
Channel::init_stereo_out_ports (bool loading)
{
  if (loading)
    {
      return;
    }

  auto l = AudioPort ("Stero out L", PortFlow::Output);
  l.id_.sym_ = "stereo_out_l";
  auto r = AudioPort ("Stereo out R", PortFlow::Output);
  r.id_.sym_ = "stereo_out_r";

  stereo_out_ = std::make_unique<StereoPorts> (std::move (l), std::move (r));
  stereo_out_->set_owner (this);
}

void
Channel::init ()
{

  /* create ports */
  switch (track_->out_signal_type_)
    {
    case PortType::Audio:
      init_stereo_out_ports (false);
      break;
    case PortType::Event:
      {
        midi_out_ =
          std::make_unique<MidiPort> (_ ("MIDI out"), PortFlow::Output);
        midi_out_->set_owner (this);
        midi_out_->id_.sym_ = "midi_out";
      }
      break;
    default:
      break;
    }

  auto fader_type = track_->get_fader_type ();
  auto prefader_type = Track::type_get_prefader_type (track_->type_);
  fader_ = std::make_unique<Fader> (fader_type, false, track_, nullptr, nullptr);
  prefader_ =
    std::make_unique<Fader> (prefader_type, true, track_, nullptr, nullptr);

  /* init sends */
  for (int i = 0; i < STRIP_SIZE; ++i)
    {
      sends_[i] =
        std::make_unique<ChannelSend> (track_->get_name_hash (), i, track_);
      sends_[i]->track_ = track_;
    }
}

void
Channel::set_phase (float phase)
{
  fader_->phase_ = phase;

  /* FIXME use an event */
  /*if (channel->widget)*/
  /*gtk_label_set_text (channel->widget->phase_reading,*/
  /*g_strdup_printf ("%.1f", phase));*/
}

float
Channel::get_phase () const
{
  return fader_->phase_;
}

void
Channel::set_balance_control (float val)
{
  fader_->balance_->set_control_value (val, false, false);
}

float
Channel::get_balance_control () const
{
  return fader_->balance_->get_control_value (false);
}

void
Channel::disconnect_plugin_from_strip (int pos, Plugin &pl)
{
  auto slot_type = pl.id_.slot_type_;
  auto prev_plugins_ptr =
    (slot_type == PluginSlotType::Insert)   ? &inserts_
    : (slot_type == PluginSlotType::MidiFx) ? &midi_fx_
    : (slot_type == PluginSlotType::Instrument)
      ? &midi_fx_
      : nullptr;
  auto next_plugins_ptr =
    (slot_type == PluginSlotType::Insert)   ? &inserts_
    : (slot_type == PluginSlotType::MidiFx) ? &midi_fx_
    : (slot_type == PluginSlotType::Instrument)
      ? &inserts_
      : nullptr;

  if (!prev_plugins_ptr || !next_plugins_ptr)
    {
      z_return_if_reached ();
    }

  auto &prev_plugins = *prev_plugins_ptr;
  auto &next_plugins = *next_plugins_ptr;

  Plugin * next_plugin = nullptr;
  for (int i = pos + 1; i < STRIP_SIZE; ++i)
    {
      if (next_plugins[i])
        {
          next_plugin = next_plugins[i].get ();
          break;
        }
    }
  if (!next_plugin && slot_type == PluginSlotType::MidiFx)
    {
      if (instrument_)
        next_plugin = instrument_.get ();
      else
        {
          for (auto &insert : inserts_)
            {
              if (insert)
                {
                  next_plugin = insert.get ();
                  break;
                }
            }
        }
    }

  Plugin * prev_plugin = nullptr;
  for (int i = pos - 1; i >= 0; --i)
    {
      if (prev_plugins[i])
        {
          prev_plugin = prev_plugins[i].get ();
          break;
        }
    }
  if (!prev_plugin && slot_type == PluginSlotType::Insert)
    {
      if (instrument_)
        prev_plugin = instrument_.get ();
      else
        {
          for (auto it = midi_fx_.rbegin (); it != midi_fx_.rend (); ++it)
            {
              if (*it)
                {
                  prev_plugin = it->get ();
                  break;
                }
            }
        }
    }

  if (!prev_plugin && !next_plugin)
    {
      disconnect_no_prev_no_next (pl);
    }
  else if (!prev_plugin && next_plugin)
    {
      disconnect_no_prev_next (pl, *next_plugin);
    }
  else if (prev_plugin && !next_plugin)
    {
      disconnect_prev_no_next (*prev_plugin, pl);
    }
  else if (prev_plugin && next_plugin)
    {
      disconnect_prev_next (*prev_plugin, pl, *next_plugin);
    }

  /* unexpose all JACK ports */
  pl.expose_ports (false, true, true);
}

std::unique_ptr<Plugin>
Channel::remove_plugin (
  PluginSlotType slot_type,
  int            slot,
  bool           moving_plugin,
  bool           deleting_plugin,
  bool           deleting_channel,
  bool           recalc_graph)
{
  std::unique_ptr<Plugin> * plugin_ptr = nullptr;
  switch (slot_type)
    {
    case PluginSlotType::Insert:
      plugin_ptr = &inserts_[slot];
      break;
    case PluginSlotType::MidiFx:
      plugin_ptr = &midi_fx_[slot];
      break;
    case PluginSlotType::Instrument:
      plugin_ptr = &instrument_;
      break;
    default:
      break;
    }
  z_return_val_if_fail (plugin_ptr, nullptr);
  auto &plugin = *plugin_ptr;
  z_return_val_if_fail (
    plugin->id_.track_name_hash_ == track_->get_name_hash (), nullptr);

  z_debug (
    "Removing {} from {}:{}:{}", plugin->get_descriptor ().name_,
    track_->get_name (), ENUM_NAME (slot_type), slot);

  /* if moving, the move is already handled in plugin_move_automation() inside
   * plugin_move(). */
  if (!moving_plugin)
    {
      plugin->remove_ats_from_automation_tracklist (
        deleting_plugin, !deleting_channel && !deleting_plugin);
    }

  if (is_in_active_project ())
    disconnect_plugin_from_strip (slot, *plugin);

  /* if deleting plugin disconnect the plugin entirely */
  if (deleting_plugin)
    {
      if (is_in_active_project () && plugin->is_selected ())
        {
          MIXER_SELECTIONS->remove_slot (plugin->id_.slot_, slot_type, true);
        }

      plugin->disconnect ();
    }

  auto raw_ptr = plugin.release ();

  if (
    track_->is_in_active_project () && !track_->disconnecting_
    && deleting_plugin && !moving_plugin)
    {
      track_->validate ();
    }

  if (recalc_graph)
    ROUTER->recalc_graph (false);

  return std::unique_ptr<Plugin> (raw_ptr);
}

Plugin *
Channel::add_plugin (
  std::unique_ptr<Plugin> &&plugin,
  PluginSlotType            slot_type,
  int                       slot,
  bool                      confirm,
  bool                      moving_plugin,
  bool                      gen_automatables,
  bool                      recalc_graph,
  bool                      pub_events)
{
  z_return_val_if_fail (
    PluginIdentifier::validate_slot_type_slot_combo (slot_type, slot), nullptr);

  bool prev_enabled = track_->enabled_;
  track_->enabled_ = false;

  auto existing_pl =
    (slot_type == PluginSlotType::Instrument) ? instrument_.get ()
    : (slot_type == PluginSlotType::Insert)
      ? inserts_[slot].get ()
      : midi_fx_[slot].get ();

  if (existing_pl)
    {
      z_debug ("existing plugin exists at {}:{}", track_->get_name (), slot);
    }

  /* free current plugin */
  if (existing_pl)
    {
      remove_plugin (slot_type, slot, moving_plugin, true, false, false);
    }

  z_debug (
    "Inserting {} {} at {}:{}:{}", ENUM_NAME (slot_type),
    plugin->get_descriptor ().name_, track_->get_name (), ENUM_NAME (slot_type),
    slot);

  Plugin * plugin_ptr = nullptr;
  if (slot_type == PluginSlotType::Instrument)
    {
      instrument_ = std::move (plugin);
      plugin_ptr = instrument_.get ();
    }
  else if (slot_type == PluginSlotType::Insert)
    {
      inserts_[slot] = std::move (plugin);
      plugin_ptr = inserts_[slot].get ();
    }
  else
    {
      midi_fx_[slot] = std::move (plugin);
      plugin_ptr = midi_fx_[slot].get ();
    }

  plugin_ptr->track_ = track_;
  plugin_ptr->set_track_and_slot (track_->get_name_hash (), slot_type, slot);

  if (track_->is_in_active_project ())
    {
      connect_plugins ();
    }

  track_->enabled_ = prev_enabled;

  if (gen_automatables)
    {
      plugin_ptr->generate_automation_tracks (*track_);
    }

  if (pub_events)
    {
      EVENTS_PUSH (EventType::ET_PLUGIN_ADDED, plugin_ptr);
    }

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  return plugin_ptr;
}

void
Channel::update_track_name_hash (
  unsigned int old_name_hash,
  unsigned int new_name_hash)
{
  /* update output */
  if (track_->is_in_active_project () && has_output_)
    {
      auto out_track = get_output_track ();
      z_return_if_fail (out_track);
      int child_idx = out_track->find_child (old_name_hash);
      z_return_if_fail (child_idx >= 0);

      out_track->children_[child_idx] = new_name_hash;
      z_debug (
        "setting output of track '{}' to '{}'", track_->get_name (),
        out_track->get_name ());
    }

  for (auto &send : sends_)
    {
      send->track_name_hash_ = new_name_hash;
    }

#define SET_PLUGIN_NAME_HASH(pl) \
  if (pl) \
  pl->set_track_name_hash (new_name_hash)

  for (auto &insert : inserts_)
    {
      SET_PLUGIN_NAME_HASH (insert);
    }
  for (auto &midi_fx : midi_fx_)
    {
      SET_PLUGIN_NAME_HASH (midi_fx);
    }
  SET_PLUGIN_NAME_HASH (instrument_);
}

AutomationTrack *
Channel::get_automation_track (PortIdentifier::Flags port_flags)
{
  auto &atl = track_->get_automation_tracklist ();
  for (auto &at : atl.ats_)
    {
      if (
        ENUM_BITSET_TEST (PortIdentifier::Flags, at->port_id_.flags_, port_flags))
        return at.get ();
    }
  return nullptr;
}

Plugin *
Channel::get_plugin_at_slot (int slot, PluginSlotType slot_type) const
{
  switch (slot_type)
    {
    case PluginSlotType::Insert:
      return inserts_[slot].get ();
    case PluginSlotType::MidiFx:
      return midi_fx_[slot].get ();
    case PluginSlotType::Instrument:
      return instrument_.get ();
    case PluginSlotType::Modulator:
    default:
      z_return_val_if_reached (nullptr);
    }
}

struct PluginImportData
{
  Channel *                ch;
  const Plugin *           pl;
  const MixerSelections *  sel;
  const PluginDescriptor * descr;
  int                      slot;
  PluginSlotType           slot_type;
  bool                     copy;
  bool                     ask_if_overwrite;
};

static void
plugin_import_data_free (void * _data)
{
  auto * self = (PluginImportData *) _data;
  object_delete_and_null (self);
}

static void
do_import (PluginImportData * data)
{
  bool plugin_valid = true;
  if (data->pl)
    {
      auto orig_track =
        TRACKLIST->find_track_by_name_hash (data->pl->id_.track_name_hash_);

      /* if plugin at original position do nothing */
      if (
        data->ch->track_ == orig_track && data->slot == data->pl->id_.slot_
        && data->slot_type == data->pl->id_.slot_type_)
        return;

      if (data->pl->setting_.descr_.is_valid_for_slot_type (
            data->slot_type, data->ch->track_->type_))
        {
          try
            {
              if (data->copy)
                {
                  UNDO_MANAGER->perform (
                    std::make_unique<MixerSelectionsCopyAction> (
                      *data->sel->gen_full_from_this (),
                      *PROJECT->port_connections_manager_, data->slot_type,
                      data->ch->track_, data->slot));
                }
              else
                {
                  UNDO_MANAGER->perform (
                    std::make_unique<MixerSelectionsMoveAction> (
                      *data->sel->gen_full_from_this (),
                      *PROJECT->port_connections_manager_, data->slot_type,
                      data->ch->track_, data->slot));
                }
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to move or copy plugins"));
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
      if (data->descr->is_valid_for_slot_type (
            data->slot_type, data->ch->track_->type_))
        {
          PluginSetting setting (*data->descr);
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<MixerSelectionsCreateAction> (
                  data->slot_type, *data->ch->track_, data->slot, setting));
              setting.increment_num_instantiations ();
            }
          catch (const ZrythmException &e)
            {
              e.handle (format_str (
                _ ("Failed to create plugin {}"), setting.get_name ()));
              return;
            }
        }
      else
        {
          plugin_valid = false;
        }
    }
  else
    {
      z_error ("No descriptor or plugin given");
      return;
    }

  if (!plugin_valid)
    {
      const auto &pl_descr =
        data->descr ? *data->descr : data->pl->setting_.descr_;
      ZrythmException e (format_str (
        _ ("Plugin {} cannot be added to this slot"), pl_descr.name_));
      e.handle (_ ("Failed to add plugin"));
    }
}

static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  auto * data = static_cast<PluginImportData *> (user_data);
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
  PluginSlotType           slot_type,
  bool                     copy,
  bool                     ask_if_overwrite)
{
  auto data = new PluginImportData ();
  data->ch = this;
  data->sel = sel;
  data->descr = descr;
  data->slot = slot;
  data->slot_type = slot_type;
  data->copy = copy;
  data->pl = pl && sel && sel->has_any () ? sel->get_first_plugin () : nullptr;

  z_info ("handling plugin import on channel {}...", track_->get_name ());

  if (ask_if_overwrite)
    {
      bool show_dialog = false;
      if (pl)
        {
          for (size_t i = 0; i < sel->slots_.size (); i++)
            {
              if (get_plugin_at_slot (slot + i, slot_type))
                {
                  show_dialog = true;
                  break;
                }
            }
        }
      else
        {
          if (get_plugin_at_slot (slot, slot_type))
            {
              show_dialog = true;
            }
        }

      if (show_dialog)
        {
          auto dialog =
            dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
          gtk_window_present (GTK_WINDOW (dialog));
          g_signal_connect_data (
            dialog, "response", G_CALLBACK (overwrite_plugin_response_cb), data,
            (GClosureNotify) plugin_import_data_free, G_CONNECT_DEFAULT);
          return;
        }
    }

  do_import (data);
  delete data;
}

void
Channel::select_all (PluginSlotType type, bool select)
{
  MIXER_SELECTIONS->clear (true);
  if (!select)
    return;

  switch (type)
    {
    case PluginSlotType::Insert:
      for (auto &insert : inserts_)
        {
          if (insert)
            {
              insert->select (true, false);
            }
        }
      break;
    case PluginSlotType::MidiFx:
      for (auto &midi_fx : midi_fx_)
        {
          if (midi_fx)
            {
              midi_fx->select (true, false);
            }
        }
      break;
    default:
      z_warning ("not implemented");
      break;
    }
}

void
Channel::set_caches ()
{
  std::vector<Plugin *> pls;
  Channel::get_plugins (pls);

  for (auto pl : pls)
    {
      pl->set_caches ();
    }
}

void
Channel::get_plugins (std::vector<Plugin *> &pls)
{
  for (auto &insert : inserts_)
    {
      if (insert)
        {
          pls.push_back (insert.get ());
        }
    }
  for (auto &midi_fx : midi_fx_)
    {
      if (midi_fx)
        {
          pls.push_back (midi_fx.get ());
        }
    }
  if (instrument_)
    {
      pls.push_back (instrument_.get ());
    }
}

void
Channel::disconnect (bool remove_pl)
{
  z_debug ("disconnecting channel {}", track_->get_name ());
  if (remove_pl)
    {
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          if (inserts_[i])
            {
              Channel::remove_plugin (
                PluginSlotType::Insert, i, false, remove_pl, false, false);
            }
          if (midi_fx_[i])
            {
              Channel::remove_plugin (
                PluginSlotType::MidiFx, i, false, remove_pl, false, false);
            }
        }
      if (instrument_)
        {
          Channel::remove_plugin (
            PluginSlotType::Instrument, 0, false, remove_pl, false, false);
        }
    }

  /* disconnect from output */
  if (is_in_active_project () && has_output_)
    {
      auto out_track = get_output_track ();
      z_return_if_fail (out_track);
      out_track->remove_child (track_->get_name_hash (), true, false, false);
    }

  /* disconnect fader/prefader */
  prefader_->disconnect_all ();
  fader_->disconnect_all ();

  /* disconnect all ports */
  std::vector<Port *> ports;
  append_ports (ports, true);
  for (auto * port : ports)
    {
      if (
        !port
        || port->is_in_active_project () != track_->is_in_active_project ())
        {
          z_error ("invalid port");
          return;
        }

      port->disconnect_all ();
    }
}