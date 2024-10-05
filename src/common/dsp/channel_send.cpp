// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/channel_send.h"
#include "gui/cpp/gtk_widgets/channel_sends_expander.h"
#include "gui/cpp/gtk_widgets/inspector_track.h"
#include "gui/cpp/gtk_widgets/left_dock_edge.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/channel_send.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/control_port.h"
#include "common/dsp/engine.h"
#include "common/dsp/graph.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/port.h"
#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/dsp.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/math.h"
#include <fmt/format.h>

PortType
ChannelSend::get_signal_type () const
{
  auto track = get_track ();
  z_return_val_if_fail (track, PortType::Audio);
  return track->out_signal_type_;
}

bool
ChannelSend::is_in_active_project () const
{
  return track_ && track_->is_in_active_project ();
}
std::string
ChannelSendTarget::describe () const
{
  switch (type)
    {
    case ChannelSendTargetType::None:
      return _ ("None");
    case ChannelSendTargetType::Track:
      {
        auto tr = TRACKLIST->get_track (track_pos);
        return tr->name_;
      }
    case ChannelSendTargetType::PluginSidechain:
      {
        auto pl = Plugin::find (pl_id);
        return pl->get_full_port_group_designation (port_group);
      }
    default:
      break;
    }
  z_return_val_if_reached (_ ("Invalid"));
}

std::string
ChannelSendTarget::get_icon () const
{
  switch (type)
    {
    case ChannelSendTargetType::None:
      return "edit-none";
    case ChannelSendTargetType::Track:
      {
        Track * tr = TRACKLIST->get_track (track_pos);
        return tr->icon_name_;
      }
    case ChannelSendTargetType::PluginSidechain:
      return "media-album-track";
    default:
      break;
    }
  z_return_val_if_reached (_ ("Invalid"));
}

void
ChannelSend::init_loaded (ChannelTrack * track)
{
  this->track_ = track;

#define INIT_LOADED_PORT(x) x->init_loaded (this)

  INIT_LOADED_PORT (enabled_);
  INIT_LOADED_PORT (amount_);
  INIT_LOADED_PORT (midi_in_);
  INIT_LOADED_PORT (midi_out_);
  stereo_in_->init_loaded (this);
  stereo_out_->init_loaded (this);

#undef INIT_LOADED_PORT
}

void
ChannelSend::construct_for_slot (int slot)
{
  slot_ = slot;

  enabled_ = std::make_unique<ControlPort> (
    format_str (_ ("Channel Send {} enabled"), slot + 1));
  enabled_->id_.sym_ = fmt::format ("channel_send_{}_enabled", slot + 1);
  enabled_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  enabled_->id_.flags2_ |= PortIdentifier::Flags2::ChannelSendEnabled;
  enabled_->set_owner<ChannelSend> (this);
  enabled_->set_control_value (0.f, false, false);

  amount_ = std::make_unique<ControlPort> (
    format_str (_ ("Channel Send {} amount"), slot + 1));
  amount_->id_.sym_ = fmt::format ("channel_send_{}_amount", slot + 1);
  amount_->id_.flags_ |= PortIdentifier::Flags::Amplitude;
  amount_->id_.flags_ |= PortIdentifier::Flags::Automatable;
  amount_->id_.flags2_ |= PortIdentifier::Flags2::ChannelSendAmount;
  amount_->set_owner (this);
  amount_->set_control_value (1.f, false, false);

  stereo_in_ = std::make_unique<StereoPorts> (
    true, format_str (_ ("Channel Send {} audio in"), slot + 1),
    fmt::format ("channel_send_{}_audio_in", slot + 1));
  stereo_in_->set_owner (this);

  midi_in_ = std::make_unique<MidiPort> (
    format_str (_ ("Channel Send {} MIDI in"), slot + 1), PortFlow::Input);
  midi_in_->id_.sym_ = fmt::format ("channel_send_{}_midi_in", slot + 1);
  midi_in_->set_owner (this);

  stereo_out_ = std::make_unique<StereoPorts> (
    false, format_str (_ ("Channel Send {} audio out"), slot + 1),
    fmt::format ("channel_send_{}_audio_out", slot + 1));
  stereo_out_->set_owner (this);

  midi_out_ = std::make_unique<MidiPort> (
    format_str (_ ("Channel Send {} MIDI out"), slot + 1), PortFlow::Output);
  midi_out_->id_.sym_ = fmt::format ("channel_send_{}_midi_out", slot + 1);
  midi_out_->set_owner (this);
}

ChannelSend::
  ChannelSend (unsigned int track_name_hash, int slot, ChannelTrack * track)
    : slot_ (slot), track_ (track), track_name_hash_ (track_name_hash)
{
  construct_for_slot (slot);
}

ChannelTrack *
ChannelSend::get_track () const
{
  z_return_val_if_fail (track_, nullptr);
  return track_;
}

bool
ChannelSend::is_target_sidechain ()
{
  return is_enabled () && is_sidechain_;
}

void
ChannelSend::prepare_process ()
{
  AudioEngine &engine = *AUDIO_ENGINE;
  if (midi_in_)
    {
      midi_in_->clear_buffer (engine);
      midi_out_->clear_buffer (engine);
    }
  if (stereo_in_)
    {
      stereo_in_->clear_buffer (engine);
      stereo_out_->clear_buffer (engine);
    }
}

void
ChannelSend::process (const nframes_t local_offset, const nframes_t nframes)
{
  if (is_empty ())
    return;

  Track * track = get_track ();
  z_return_if_fail (track);
  if (track->out_signal_type_ == PortType::Audio)
    {
      if (math_floats_equal_epsilon (amount_->control_, 1.f, 0.00001f))
        {
          dsp_copy (
            &stereo_out_->get_l ().buf_[local_offset],
            &stereo_in_->get_l ().buf_[local_offset], nframes);
          dsp_copy (
            &stereo_out_->get_r ().buf_[local_offset],
            &stereo_in_->get_r ().buf_[local_offset], nframes);
        }
      else
        {
          dsp_mix2 (
            &stereo_out_->get_l ().buf_[local_offset],
            &stereo_in_->get_l ().buf_[local_offset], 1.f, amount_->control_,
            nframes);
          dsp_mix2 (
            &stereo_out_->get_r ().buf_[local_offset],
            &stereo_in_->get_r ().buf_[local_offset], 1.f, amount_->control_,
            nframes);
        }
    }
  else if (track->out_signal_type_ == PortType::Event)
    {
      midi_out_->midi_events_.active_events_.append (
        midi_in_->midi_events_.active_events_, local_offset, nframes);
    }
}

void
ChannelSend::copy_values_from (const ChannelSend &other)
{
  slot_ = other.slot_;
  enabled_->set_control_value (other.enabled_->control_, false, false);
  amount_->set_control_value (other.amount_->control_, false, false);
  is_sidechain_ = other.is_sidechain_;
}

Track *
ChannelSend::get_target_track (const ChannelTrack * owner)
{
  if (is_empty ())
    return nullptr;

  PortType                      type = get_signal_type ();
  std::optional<PortConnection> conn;
  switch (type)
    {
    case PortType::Audio:
      conn = PORT_CONNECTIONS_MGR->get_source_or_dest (
        stereo_out_->get_l ().id_, false);
      break;
    case PortType::Event:
      conn = PORT_CONNECTIONS_MGR->get_source_or_dest (midi_out_->id_, false);
      break;
    default:
      z_return_val_if_reached (nullptr);
      break;
    }

  z_return_val_if_fail (conn, nullptr);
  Port * port = Port::find_from_identifier (conn->dest_id_);
  z_return_val_if_fail (IS_PORT_AND_NONNULL (port), nullptr);

  return port->get_track (true);
}

std::unique_ptr<StereoPorts>
ChannelSend::get_target_sidechain ()
{
  z_return_val_if_fail (!is_empty () && is_sidechain_, nullptr);

  PortType type = get_signal_type ();
  z_return_val_if_fail (type == PortType::Audio, nullptr);

  auto conn =
    PORT_CONNECTIONS_MGR->get_source_or_dest (stereo_out_->get_l ().id_, false);
  z_return_val_if_fail (conn, nullptr);
  auto * l = Port::find_from_identifier<AudioPort> (conn->dest_id_);
  z_return_val_if_fail (l, nullptr);

  conn =
    PORT_CONNECTIONS_MGR->get_source_or_dest (stereo_out_->get_r ().id_, false);
  z_return_val_if_fail (conn, nullptr);
  auto * r = Port::find_from_identifier<AudioPort> (conn->dest_id_);
  z_return_val_if_fail (r, nullptr);

  return std::make_unique<StereoPorts> (l->clone_unique (), r->clone_unique ());
}

void
ChannelSend::connect_to_owner ()
{
  PortType type = get_signal_type ();
  switch (type)
    {
    case PortType::Audio:
      for (int i = 0; i < 2; i++)
        {
          Port &self_port = i == 0 ? stereo_in_->get_l () : stereo_in_->get_r ();
          Port * src_port = nullptr;
          if (is_prefader ())
            {
              src_port =
                i == 0
                  ? &track_->channel_->prefader_->stereo_out_->get_l ()
                  : &track_->channel_->prefader_->stereo_out_->get_r ();
            }
          else
            {
              src_port =
                i == 0
                  ? &track_->channel_->fader_->stereo_out_->get_l ()
                  : &track_->channel_->fader_->stereo_out_->get_r ();
            }

          /* make the connection if not exists */
          PORT_CONNECTIONS_MGR->ensure_connect (
            src_port->id_, self_port.id_, 1.f, true, true);
        }
      break;
    case PortType::Event:
      {
        Port * self_port = midi_in_.get ();
        Port * src_port = nullptr;
        if (is_prefader ())
          {
            src_port = track_->channel_->prefader_->midi_out_.get ();
          }
        else
          {
            src_port = track_->channel_->fader_->midi_out_.get ();
          }

        /* make the connection if not exists */
        PORT_CONNECTIONS_MGR->ensure_connect (
          src_port->id_, self_port->id_, 1.f, true, true);
      }
      break;
    default:
      break;
    }
}

float
ChannelSend::get_amount_for_widgets () const
{
  z_return_val_if_fail (is_enabled (), 0.f);

  return math_get_fader_val_from_amp (amount_->control_);
}

void
ChannelSend::set_amount_from_widget (float val)
{
  z_return_if_fail (is_enabled ());

  set_amount (math_get_amp_val_from_fader (val));
}

bool
ChannelSend::connect_stereo (
  StereoPorts * stereo,
  AudioPort *   l,
  AudioPort *   r,
  bool          sidechain,
  bool          recalc_graph,
  bool          validate)
{
  /* verify can be connected */
  if (validate && l->is_in_active_project ())
    {
      auto src =
        Port::find_from_identifier<AudioPort> (stereo_out_->get_l ().id_);
      if (!src->can_be_connected_to (*l))
        {
          throw ZrythmException (_ ("Ports cannot be connected"));
        }
    }

  disconnect (false);

  /* connect */
  if (stereo)
    {
      l = &stereo->get_l ();
      r = &stereo->get_r ();
    }

  PORT_CONNECTIONS_MGR->ensure_connect (
    stereo_out_->get_l ().id_, l->id_, 1.f, true, true);
  PORT_CONNECTIONS_MGR->ensure_connect (
    stereo_out_->get_r ().id_, r->id_, 1.f, true, true);

  enabled_->set_control_value (1.f, false, true);
  is_sidechain_ = sidechain;

#if 0
  /* set multipliers */
  update_connections ();
#endif

  if (recalc_graph)
    ROUTER->recalc_graph (false);

  return true;
}

bool
ChannelSend::connect_midi (MidiPort &port, bool recalc_graph, bool validate)
{
  /* verify can be connected */
  if (validate && port.is_in_active_project ())
    {
      Port * src = Port::find_from_identifier (midi_out_->id_);
      if (!src->can_be_connected_to (port))
        {
          throw ZrythmException (_ ("Ports cannot be connected"));
        }
    }

  disconnect (false);

  PORT_CONNECTIONS_MGR->ensure_connect (
    midi_out_->id_, port.id_, 1.f, true, true);

  enabled_->set_control_value (1.f, false, true);

  if (recalc_graph)
    ROUTER->recalc_graph (false);

  return true;
}

void
ChannelSend::disconnect_midi ()
{
  const auto conn =
    PORT_CONNECTIONS_MGR->get_source_or_dest (midi_out_->id_, false);
  if (!conn)
    return;

  auto * dest_port = Port::find_from_identifier (conn->dest_id_);
  z_return_if_fail (dest_port);

  PORT_CONNECTIONS_MGR->ensure_disconnect (midi_out_->id_, dest_port->id_);
}

void
ChannelSend::disconnect_audio ()
{
  for (int i = 0; i < 2; i++)
    {
      auto * src_port = i == 0 ? &stereo_out_->get_l () : &stereo_out_->get_r ();
      const auto conn =
        PORT_CONNECTIONS_MGR->get_source_or_dest (src_port->id_, false);
      if (!conn)
        continue;

      auto * dest_port = Port::find_from_identifier (conn->dest_id_);
      z_return_if_fail (dest_port);
      PORT_CONNECTIONS_MGR->ensure_disconnect (src_port->id_, dest_port->id_);
    }
}

/**
 * Removes the connection at the given send.
 */
void
ChannelSend::disconnect (bool recalc_graph)
{
  if (is_empty ())
    return;

  PortType signal_type = get_signal_type ();

  switch (signal_type)
    {
    case PortType::Audio:
      disconnect_audio ();
      break;
    case PortType::Event:
      disconnect_midi ();
      break;
    default:
      break;
    }

  enabled_->set_control_value (0.f, false, true);
  is_sidechain_ = false;

  if (recalc_graph)
    ROUTER->recalc_graph (false);
}

void
ChannelSend::set_amount (float amount)
{
  amount_->set_control_value (amount, false, true);
}

/**
 * Get the name of the destination, or a placeholder
 * text if empty.
 */
std::string
ChannelSend::get_dest_name () const
{
  if (is_empty ())
    {
      if (is_prefader ())
        return _ ("Pre-fader send");

      return _ ("Post-fader send");
    }
  else
    {
      PortType    type = get_signal_type ();
      const Port &search_port =
        (type == PortType::Audio)
          ? static_cast<Port &> (stereo_out_->get_l ())
          : static_cast<Port &> (*midi_out_);
      const auto conn =
        PORT_CONNECTIONS_MGR->get_source_or_dest (search_port.id_, false);
      z_return_val_if_fail (conn, {});
      auto * dest = Port::find_from_identifier (conn->dest_id_);
      z_return_val_if_fail (dest, {});
      if (is_sidechain_)
        {
          auto * pl = dest->get_plugin (true);
          z_return_val_if_fail (pl, {});
          return pl->get_full_port_group_designation (dest->id_.port_group_);
        }
      else /* else if not sidechain */
        {
          switch (dest->id_.owner_type_)
            {
            case PortIdentifier::OwnerType::TrackProcessor:
              {
                auto * track = dest->get_track (true);
                z_return_val_if_fail (track, {});
                return format_str (_ ("{} input"), track->name_);
              }
              break;
            default:
              break;
            }
        }
    }
  z_return_val_if_reached ({});
}

void
ChannelSend::init_after_cloning (const ChannelSend &other)
{
  slot_ = other.slot_;
  is_sidechain_ = other.is_sidechain_;
  track_name_hash_ = other.track_name_hash_;
  enabled_ = other.enabled_->clone_unique ();
  amount_ = other.amount_->clone_unique ();
  stereo_in_ = other.stereo_in_->clone_unique ();
  midi_in_ = other.midi_in_->clone_unique ();
  stereo_out_ = other.stereo_out_->clone_unique ();
  midi_out_ = other.midi_out_->clone_unique ();

  std::vector<Port *> ports;
  append_ports (ports);
  for (auto port : ports)
    {
      port->set_owner (this);
    }
}

bool
ChannelSend::is_enabled () const
{
  if (ZRYTHM_TESTING)
    {
      z_return_val_if_fail (enabled_, false);
    }

  bool enabled = enabled_->is_toggled ();

  if (!enabled)
    return false;

  PortType    signal_type = get_signal_type ();
  const Port &search_port =
    (signal_type == PortType::Audio)
      ? static_cast<Port &> (stereo_out_->get_l ())
      : *midi_out_;

  if (ROUTER->is_processing_thread ()) [[likely]]
    {
      if (search_port.dests_.size () == 1)
        {
          auto * dest = search_port.dests_[0];
          z_return_val_if_fail (IS_PORT_AND_NONNULL (dest), false);

          if (dest->id_.owner_type_ == PortIdentifier::OwnerType::Plugin)
            {
              auto * pl = Plugin::find (dest->id_.plugin_id_);
              z_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);
              if (pl->instantiation_failed_)
                return false;
            }

          return true;
        }
      else
        return false;
    }

  /* get dest port */
  const auto conn =
    PORT_CONNECTIONS_MGR->get_source_or_dest (search_port.id_, false);
  z_return_val_if_fail (conn, false);
  auto * dest = Port::find_from_identifier (conn->dest_id_);
  z_return_val_if_fail (IS_PORT_AND_NONNULL (dest), false);

  /* if dest port is a plugin port and plugin instantiation failed, assume that
   * the send is disabled */
  if (dest->id_.owner_type_ == PortIdentifier::OwnerType::Plugin)
    {
      auto * pl = Plugin::find (dest->id_.plugin_id_);
      if (pl->instantiation_failed_)
        enabled = false;
    }

  return enabled;
}

ChannelSendWidget *
ChannelSend::find_widget ()
{
  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      return MW_TRACK_INSPECTOR->sends->slots[slot_];
    }
  return nullptr;
}

ChannelSend *
ChannelSend::find_in_project () const
{
  auto * track =
    TRACKLIST->find_track_by_name_hash<ChannelTrack> (track_name_hash_);
  z_return_val_if_fail (track, nullptr);

  return track->channel_->sends_[slot_].get ();
}

bool
ChannelSend::validate ()
{
  if (is_enabled ())
    {
      PortType signal_type = get_signal_type ();
      if (signal_type == PortType::Audio)
        {
          int num_dests = PORT_CONNECTIONS_MGR->get_sources_or_dests (
            nullptr, stereo_out_->get_l ().id_, false);
          z_return_val_if_fail (num_dests == 1, false);
          num_dests = PORT_CONNECTIONS_MGR->get_sources_or_dests (
            nullptr, stereo_out_->get_r ().id_, false);
          z_return_val_if_fail (num_dests == 1, false);
        }
      else if (signal_type == PortType::Event)
        {
          int num_dests = PORT_CONNECTIONS_MGR->get_sources_or_dests (
            nullptr, midi_out_->id_, false);
          z_return_val_if_fail (num_dests == 1, false);
        }
    } /* endif channel send is enabled */

  return true;
}

void
ChannelSend::append_ports (std::vector<Port *> &ports)
{
  auto _add = [&ports] (Port &port) { ports.push_back (&port); };

  _add (*enabled_);
  _add (*amount_);
  if (midi_in_)
    {
      _add (*midi_in_);
    }
  if (midi_out_)
    {
      _add (*midi_out_);
    }
  if (stereo_in_)
    {
      _add (stereo_in_->get_l ());
      _add (stereo_in_->get_r ());
    }
  if (stereo_out_)
    {
      _add (stereo_out_->get_l ());
      _add (stereo_out_->get_r ());
    }
}

int
ChannelSend::append_connection (
  const PortConnectionsManager * mgr,
  std::vector<PortConnection>   &arr) const
{
  if (is_empty ())
    return 0;

  PortType signal_type = get_signal_type ();
  if (signal_type == PortType::Audio)
    {
      int num_dests =
        mgr->get_sources_or_dests (&arr, stereo_out_->get_l ().id_, false);
      z_return_val_if_fail (num_dests == 1, false);
      num_dests =
        mgr->get_sources_or_dests (&arr, stereo_out_->get_r ().id_, false);
      z_return_val_if_fail (num_dests == 1, false);
      return 2;
    }
  else if (signal_type == PortType::Event)
    {
      int num_dests = mgr->get_sources_or_dests (&arr, midi_out_->id_, false);
      z_return_val_if_fail (num_dests == 1, false);
      return 1;
    }

  z_return_val_if_reached (0);
}

bool
ChannelSend::is_connected_to (const StereoPorts * stereo, const Port * midi) const
{
  std::vector<PortConnection> conns;
  int num_conns = append_connection (PORT_CONNECTIONS_MGR.get (), conns);
  for (int i = 0; i < num_conns; i++)
    {
      const auto &conn = conns[i];
      if (((stereo != nullptr) && (conn.dest_id_ == stereo->get_l ().id_ || conn.dest_id_ == stereo->get_r ().id_)) ||
          ((midi != nullptr) && conn.dest_id_ == midi->id_))
        {
          return true;
        }
    }

  return false;
}