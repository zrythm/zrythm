// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "engine/device_io/engine.h"
#include "engine/session/project_graph_builder.h"
#include "engine/session/router.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/port.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/dsp.h"
#include "utils/math.h"

#include <fmt/format.h>

namespace zrythm::structure::tracks
{

ChannelSend::ChannelSend (
  TrackRegistry            &track_registry,
  PortRegistry             &port_registry,
  OptionalRef<ChannelTrack> track,
  std::optional<int>        slot,
  bool                      create_ports)
    : port_registry_ (port_registry), track_registry_ (track_registry)
{
  if (slot)
    {
      slot_ = slot.value ();
    }
  if (track)
    {
      track_ = track;
      track_id_ = track->get_uuid ();
    }
  if (create_ports)
    {
      assert (track);
      construct_for_slot (*track, slot_);
    }
}

structure::tracks::PortType
ChannelSend::get_signal_type () const
{
  auto track = get_track ();
  z_return_val_if_fail (track, PortType::Audio);
  return track->get_output_signal_type ();
}

utils::Utf8String
ChannelSendTarget::describe () const
{
  switch (type)
    {
    case ChannelSendTargetType::None:
      return utils::Utf8String::from_qstring (QObject::tr ("None"));
    case ChannelSendTargetType::Track:
      {
        auto tr =
          Track::from_variant (TRACKLIST->get_track_at_index (track_pos));
        return tr->get_name ();
      }
    case ChannelSendTargetType::PluginSidechain:
      {
        auto pl_var = PROJECT->find_plugin_by_id (pl_id);
        return std::visit (
          [&] (auto &&pl) {
            return pl->get_full_port_group_designation (port_group);
          },
          *pl_var);
      }
    default:
      break;
    }
  z_return_val_if_reached (
    utils::Utf8String::from_qstring (QObject::tr ("Invalid")));
}

utils::Utf8String
ChannelSendTarget::get_icon () const
{
  switch (type)
    {
    case ChannelSendTargetType::None:
      return u8"edit-none";
    case ChannelSendTargetType::Track:
      {
        Track * tr =
          Track::from_variant (TRACKLIST->get_track_at_index (track_pos));
        return tr->get_icon_name ();
      }
    case ChannelSendTargetType::PluginSidechain:
      return u8"media-album-track";
    default:
      break;
    }
  z_return_val_if_reached (
    utils::Utf8String::from_qstring (QObject::tr ("Invalid")));
}

void
ChannelSend::init_loaded (ChannelTrack * track)
{
  get_enabled_port ().init_loaded (*this);
  get_amount_port ().init_loaded (*this);
  if (is_midi ())
    {
      get_midi_in_port ().init_loaded (*this);
      get_midi_out_port ().init_loaded (*this);
    }
  else if (is_audio ())
    {
      get_stereo_in_ports ().first.init_loaded (*this);
      get_stereo_in_ports ().second.init_loaded (*this);
      get_stereo_out_ports ().first.init_loaded (*this);
      get_stereo_out_ports ().second.init_loaded (*this);
    }
}

void
ChannelSend::construct_for_slot (ChannelTrack &track, int slot)
{
  slot_ = slot;

  enabled_id_ = get_port_registry ().create_object<ControlPort> (
    utils::Utf8String::from_qstring (
      format_qstr (QObject::tr ("Channel Send {} enabled"), slot + 1)));
  auto &enabled_port = get_enabled_port ();
  enabled_port.id_->sym_ = utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("channel_send_{}_enabled", slot + 1));
  enabled_port.id_->flags_ |= structure::tracks::PortIdentifier::Flags::Toggle;
  enabled_port.id_->flags2_ |=
    structure::tracks::PortIdentifier::Flags2::ChannelSendEnabled;
  enabled_port.set_owner (*this);
  enabled_port.set_control_value (0.f, false, false);

  amount_id_ = get_port_registry ().create_object<ControlPort> (
    utils::Utf8String::from_qstring (
      format_qstr (QObject::tr ("Channel Send {} amount"), slot + 1)));
  auto &amount_port = get_amount_port ();

  amount_port.id_->sym_ = utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("channel_send_{}_amount", slot + 1));
  amount_port.id_->flags_ |= structure::tracks::PortIdentifier::Flags::Amplitude;
  amount_port.id_->flags_ |=
    structure::tracks::PortIdentifier::Flags::Automatable;
  amount_port.id_->flags2_ |=
    structure::tracks::PortIdentifier::Flags2::ChannelSendAmount;
  amount_port.set_owner (*this);
  amount_port.set_control_value (1.f, false, false);

  if (is_audio ())
    {
      {
        auto stereo_in_ports = StereoPorts::create_stereo_ports (
          port_registry_, true,
          utils::Utf8String::from_qstring (format_qstr (

            QObject::tr ("Channel Send {} audio in"), slot + 1)),
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("channel_send_{}_audio_in", slot + 1)));
        stereo_in_left_id_ = stereo_in_ports.first;
        stereo_in_right_id_ = stereo_in_ports.second;
        auto &left_port = get_stereo_in_ports ().first;
        auto &right_port = get_stereo_in_ports ().second;
        left_port.set_owner (*this);
        right_port.set_owner (*this);
      }

      {
        auto stereo_out_ports = StereoPorts::create_stereo_ports (
          port_registry_, false,
          utils::Utf8String::from_qstring (
            format_qstr (QObject::tr ("Channel Send {} audio out"), slot + 1)),
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("channel_send_{}_audio_out", slot + 1)));
        stereo_out_left_id_ = stereo_out_ports.first;
        stereo_out_right_id_ = stereo_out_ports.second;
        auto &left_port = get_stereo_out_ports ().first;
        auto &right_port = get_stereo_out_ports ().second;
        left_port.set_owner (*this);
        right_port.set_owner (*this);
      }
    }
  else if (is_midi ())
    {
      midi_in_id_ = get_port_registry ().create_object<MidiPort> (
        utils::Utf8String::from_qstring (
          format_qstr (QObject::tr ("Channel Send {} MIDI in"), slot + 1)),
        structure::tracks::PortFlow::Input);
      auto &midi_in_port = get_midi_in_port ();
      midi_in_port.id_->sym_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("channel_send_{}_midi_in", slot + 1));
      midi_in_port.set_owner (*this);

      midi_out_id_ = get_port_registry ().create_object<MidiPort> (
        utils::Utf8String::from_qstring (
          format_qstr (QObject::tr ("Channel Send {} MIDI out"), slot + 1)),
        structure::tracks::PortFlow::Output);
      auto &midi_out_port = get_midi_out_port ();
      midi_out_port.id_->sym_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("channel_send_{}_midi_out", slot + 1));
      midi_out_port.set_owner (*this);
    }
}

ChannelTrack *
ChannelSend::get_track () const
{
  if (track_)
    {
      // FIXME const cast
      return const_cast<ChannelTrack *> (std::addressof (*track_));
    }
  return std::visit (
    [&] (auto &&track) -> ChannelTrack * {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        return track;
      throw std::runtime_error (
        "ChannelSend::get_track(): track is not a ChannelTrack");
    },
    track_registry_.find_by_id_or_throw (track_id_));
}

bool
ChannelSend::is_target_sidechain ()
{
  return is_enabled () && is_sidechain_;
}

void
ChannelSend::prepare_process (std::size_t block_length)
{
  if (is_midi ())
    {
      get_midi_in_port ().clear_buffer (block_length);
      get_midi_out_port ().clear_buffer (block_length);
    }
  if (is_audio ())
    {
      get_stereo_in_ports ().first.clear_buffer (block_length);
      get_stereo_in_ports ().second.clear_buffer (block_length);
      get_stereo_out_ports ().first.clear_buffer (block_length);
      get_stereo_out_ports ().second.clear_buffer (block_length);
    }
}

utils::Utf8String
ChannelSend::get_node_name () const
{
  auto * tr = get_track ();
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/Channel Send {}", tr->get_name (), slot_ + 1));
}

void
ChannelSend::process_block (const EngineProcessTimeInfo time_nfo)
{
  if (is_empty ())
    return;

  const auto local_offset = time_nfo.local_offset_;
  const auto nframes = time_nfo.nframes_;
  Track *    track = get_track ();
  z_return_if_fail (track);
  if (track->get_output_signal_type () == PortType::Audio)
    {
      const auto amount_val = get_amount_value ();
      if (utils::math::floats_near (amount_val, 1.f, 0.00001f))
        {
          utils::float_ranges::copy (
            &get_stereo_out_ports ().first.buf_[local_offset],
            &get_stereo_in_ports ().first.buf_[local_offset], nframes);
          utils::float_ranges::copy (
            &get_stereo_out_ports ().second.buf_[local_offset],
            &get_stereo_in_ports ().second.buf_[local_offset], nframes);
        }
      else
        {
          utils::float_ranges::mix_product (
            &get_stereo_out_ports ().first.buf_[local_offset],
            &get_stereo_in_ports ().first.buf_[local_offset], amount_val,
            nframes);
          utils::float_ranges::mix_product (
            &get_stereo_out_ports ().second.buf_[local_offset],
            &get_stereo_in_ports ().second.buf_[local_offset], amount_val,
            nframes);
        }
    }
  else if (track->get_output_signal_type () == PortType::Event)
    {
      get_midi_out_port ().midi_events_.active_events_.append (
        get_midi_in_port ().midi_events_.active_events_, local_offset, nframes);
    }
}

void
ChannelSend::copy_values_from (const ChannelSend &other)
{
  slot_ = other.slot_;
  get_enabled_port ().set_control_value (
    other.get_enabled_port ().control_, false, false);
  get_amount_port ().set_control_value (
    other.get_amount_port ().control_, false, false);
  is_sidechain_ = other.is_sidechain_;
}

Track *
ChannelSend::get_target_track ()
{
  if (is_empty ())
    return nullptr;

  auto * mgr = get_port_connections_manager ();
  z_return_val_if_fail (mgr, nullptr);

  PortType         type = get_signal_type ();
  PortConnection * conn = nullptr;
  switch (type)
    {
    case PortType::Audio:
      conn = mgr->get_source_or_dest (stereo_out_left_id_->id (), false);
      break;
    case PortType::Event:
      conn = mgr->get_source_or_dest (midi_out_id_->id (), false);
      break;
    default:
      z_return_val_if_reached (nullptr);
      break;
    }

  z_return_val_if_fail (conn, nullptr);
  auto port_var = get_port_registry ().find_by_id (conn->dest_id_);
  z_return_val_if_fail (port_var, nullptr);
  auto ret = std::visit (
    [&] (auto &&port) {
      return track_registry_.find_by_id_or_throw (
        port->id_->get_track_id ().value ());
    },
    port_var->get ());
  return std::visit ([&] (auto &&track) -> Track * { return track; }, ret);
}

void
ChannelSend::connect_to_owner ()
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  auto channel = get_track ()->get_channel ();
  if (is_audio ())
    {
      for (int i = 0; i < 2; i++)
        {
          auto self_port_id =
            i == 0 ? stereo_in_left_id_->id () : stereo_in_right_id_->id ();
          std::optional<PortUuid> src_port_id;
          if (is_prefader ())
            {
              src_port_id =
                i == 0
                  ? channel->prefader_->get_stereo_out_left_id ()
                  : channel->prefader_->get_stereo_out_right_id ();
            }
          else
            {
              src_port_id =
                i == 0
                  ? channel->fader_->get_stereo_out_left_id ()
                  : channel->fader_->get_stereo_out_right_id ();
            }

          /* make the connection if not exists */
          mgr->add_connection (
            src_port_id.value (), self_port_id, 1.f, true, true);
        }
    }
  else if (is_midi ())
    {
      auto                    self_port_id = midi_in_id_->id ();
      std::optional<PortUuid> src_port_id;
      if (is_prefader ())
        {
          src_port_id = channel->prefader_->get_midi_out_id ();
        }
      else
        {
          src_port_id = channel->fader_->get_midi_out_id ();
        }

      /* make the connection if not exists */
      mgr->add_connection (src_port_id.value (), self_port_id, 1.f, true, true);
    }
}

float
ChannelSend::get_amount_for_widgets () const
{
  z_return_val_if_fail (is_enabled (), 0.f);

  return utils::math::get_fader_val_from_amp (get_amount_value ());
}

void
ChannelSend::set_amount_from_widget (float val)
{
  z_return_if_fail (is_enabled ());

  set_amount (utils::math::get_amp_val_from_fader (val));
}

bool
ChannelSend::connect_stereo (
  AudioPort &l,
  AudioPort &r,
  bool       sidechain,
  bool       recalc_graph,
  bool       validate)
{
  auto * mgr = get_port_connections_manager ();
  z_return_val_if_fail (mgr, false);

  /* verify can be connected */
  if (validate)
    {
      const auto &src = get_stereo_out_ports ().first;
      if (!ProjectGraphBuilder::can_ports_be_connected (*PROJECT, src, l))
        {
          throw ZrythmException (QObject::tr ("Ports cannot be connected"));
        }
    }

  disconnect (false);

  /* connect */
  mgr->add_connection (
    stereo_out_left_id_->id (), l.get_uuid (), 1.f, true, true);
  mgr->add_connection (
    stereo_out_right_id_->id (), r.get_uuid (), 1.f, true, true);

  get_enabled_port ().set_control_value (1.f, false, true);
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
  auto * mgr = get_port_connections_manager ();
  z_return_val_if_fail (mgr, false);

  /* verify can be connected */
  if (validate)
    {
      const auto &src = get_midi_out_port ();
      if (!ProjectGraphBuilder::can_ports_be_connected (*PROJECT, src, port))
        {
          throw ZrythmException (QObject::tr ("Ports cannot be connected"));
        }
    }

  disconnect (false);

  mgr->add_connection (midi_out_id_->id (), port.get_uuid (), 1.f, true, true);

  get_enabled_port ().set_control_value (1.f, false, true);

  if (recalc_graph)
    ROUTER->recalc_graph (false);

  return true;
}

void
ChannelSend::disconnect_midi ()
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  auto * const conn = mgr->get_source_or_dest (midi_out_id_->id (), false);
  if (!conn)
    return;

  auto dest_port_var = get_port_registry ().find_by_id (conn->dest_id_);
  z_return_if_fail (
    dest_port_var && std::holds_alternative<MidiPort *> (dest_port_var->get ()));
  auto * dest_port = std::get<MidiPort *> (dest_port_var->get ());

  mgr->remove_connection (midi_out_id_->id (), dest_port->get_uuid ());
}

void
ChannelSend::disconnect_audio ()
{
  auto * mgr = get_port_connections_manager ();
  z_return_if_fail (mgr);

  for (int i = 0; i < 2; i++)
    {
      auto src_port_id =
        i == 0 ? stereo_out_left_id_->id () : stereo_out_right_id_->id ();
      auto * const conn = mgr->get_source_or_dest (src_port_id, false);
      if (!conn)
        continue;

      auto dest_port_var = get_port_registry ().find_by_id (conn->dest_id_);
      z_return_if_fail (
        dest_port_var
        && std::holds_alternative<AudioPort *> (dest_port_var->get ()));
      auto * dest_port = std::get<AudioPort *> (dest_port_var->get ());
      mgr->remove_connection (src_port_id, dest_port->get_uuid ());
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

  get_enabled_port ().set_control_value (0.f, false, true);
  is_sidechain_ = false;

  if (recalc_graph)
    ROUTER->recalc_graph (false);
}

structure::tracks::PortConnectionsManager *
ChannelSend::get_port_connections_manager () const
{
  auto * track = get_track ();
  z_return_val_if_fail (track, nullptr);
  auto * mgr = track->get_port_connections_manager ();
  z_return_val_if_fail (mgr, nullptr);
  return mgr;
}

void
ChannelSend::set_amount (float amount)
{
  get_amount_port ().set_control_value (amount, false, true);
}

utils::Utf8String
ChannelSend::get_dest_name () const
{
  auto * mgr = get_port_connections_manager ();
  z_return_val_if_fail (mgr, {});

  if (is_empty ())
    {
      if (is_prefader ())
        return utils::Utf8String::from_qstring (QObject::tr ("Pre-fader send"));

      return utils::Utf8String::from_qstring (QObject::tr ("Post-fader send"));
    }

  const auto &search_port_id =
    is_audio () ? stereo_out_left_id_->id () : midi_out_id_->id ();
  auto * const conn = mgr->get_source_or_dest (search_port_id, false);
  z_return_val_if_fail (conn, {});
  auto dest_var = get_port_registry ().find_by_id_or_throw (conn->dest_id_);
  return std::visit (
    [&] (auto &&dest) -> utils::Utf8String {
      z_return_val_if_fail (dest, {});
      if (is_sidechain_)
        {
          auto pl_var =
            PROJECT->find_plugin_by_id (dest->id_->plugin_id_.value ());
          z_return_val_if_fail (pl_var.has_value (), {});
          return std::visit (
            [&] (auto &&pl) {
              return pl->get_full_port_group_designation (
                *dest->id_->port_group_);
            },
            pl_var.value ());
        }
      /* else if not sidechain */
      switch (dest->id_->owner_type_)
        {
        case structure::tracks::PortIdentifier::OwnerType::TrackProcessor:
          {
            auto track_var = track_registry_.find_by_id_or_throw (
              dest->id_->get_track_id ().value ());
            return std::visit (
              [&] (auto &&track) {
                return utils::Utf8String::from_qstring (
                  format_qstr (QObject::tr ("{} input"), track->get_name ()));
              },
              track_var);
          }
          break;
        default:
          break;
        }

      z_return_val_if_reached ({});
    },
    dest_var);
}

utils::Utf8String
ChannelSend::get_full_designation_for_port (
  const structure::tracks::PortIdentifier &id) const
{
  auto * tr = get_track ();
  z_return_val_if_fail (tr, {});
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", tr->get_name (), id.get_label ()));
}

void
init_from (
  ChannelSend           &obj,
  const ChannelSend     &other,
  utils::ObjectCloneType clone_type)
{
  obj.slot_ = other.slot_;
  obj.is_sidechain_ = other.is_sidechain_;
  obj.track_id_ = other.track_id_;
  if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
      auto deep_clone_port = [&] (auto &own_port_id, const auto &other_port_id) {
        if (!other_port_id.has_value ())
          return;

        auto other_amp_port = other_port_id->get_object ();
        std::visit (
          [&] (auto &&other_port) {
            own_port_id = obj.port_registry_.clone_object (*other_port);
          },
          other_amp_port);
      };

      deep_clone_port (obj.enabled_id_, other.enabled_id_);
      deep_clone_port (obj.amount_id_, other.amount_id_);
      deep_clone_port (obj.stereo_in_left_id_, other.stereo_in_left_id_);
      deep_clone_port (obj.stereo_in_right_id_, other.stereo_in_right_id_);
      deep_clone_port (obj.midi_in_id_, other.midi_in_id_);
      deep_clone_port (obj.stereo_out_left_id_, other.stereo_out_left_id_);
      deep_clone_port (obj.stereo_out_right_id_, other.stereo_out_right_id_);
      deep_clone_port (obj.midi_out_id_, other.midi_out_id_);

      // set owner
      std::vector<Port *> ports;
      obj.append_ports (ports);
      for (auto * port : ports)
        {
          port->set_owner (obj);
        }
    }
  else if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.enabled_id_ = other.enabled_id_;
      obj.amount_id_ = other.amount_id_;
      obj.stereo_in_left_id_ = other.stereo_in_left_id_;
      obj.stereo_in_right_id_ = other.stereo_in_right_id_;
      obj.midi_in_id_ = other.midi_in_id_;
      obj.stereo_out_left_id_ = other.stereo_out_left_id_;
      obj.stereo_out_right_id_ = other.stereo_out_right_id_;
      obj.midi_out_id_ = other.midi_out_id_;
    }
}

bool
ChannelSend::is_enabled () const
{
  assert (enabled_id_.has_value ());
  bool enabled = get_enabled_port ().is_toggled ();

  if (!enabled)
    return false;

  auto * mgr = get_port_connections_manager ();
  z_return_val_if_fail (mgr, false);

  const Port &search_port =
    is_audio ()
      ? static_cast<const Port &> (get_stereo_out_ports ().first)
      : get_midi_out_port ();

  if (ROUTER->is_processing_thread ()) [[likely]]
    {
      if (search_port.dests_.size () == 1)
        {
          auto * dest = search_port.dests_[0];
          z_return_val_if_fail (dest, false);

          if (
            dest->id_->owner_type_
            == structure::tracks::PortIdentifier::OwnerType::Plugin)
            {
              auto pl_var =
                PROJECT->find_plugin_by_id (dest->id_->plugin_id_.value ());
              z_return_val_if_fail (pl_var.has_value (), false);
              auto instantiation_failed = std::visit (
                [&] (auto &&pl) { return pl->instantiation_failed_; },
                pl_var.value ());
              if (instantiation_failed)
                {
                  return false;
                }
            }

          return true;
        }
      return false;
    }

  /* get dest port */
  const auto conn = mgr->get_source_or_dest (search_port.get_uuid (), false);
  z_return_val_if_fail (conn, false);
  auto dest_var = get_port_registry ().find_by_id_or_throw (conn->dest_id_);
  return std::visit (
    [&] (auto &&dest) {
      /* if dest port is a plugin port and plugin instantiation failed, assume
       * that the send is disabled */
      if (
        dest->id_->owner_type_
        == structure::tracks::PortIdentifier::OwnerType::Plugin)
        {
          auto pl_var =
            PROJECT->find_plugin_by_id (dest->id_->plugin_id_.value ());
          std::visit (
            [&] (auto &&pl) {
              if (pl->instantiation_failed_)
                enabled = false;
            },
            pl_var.value ());
        }

      return enabled;
    },
    dest_var);
}

#if 0
ChannelSendWidget *
ChannelSend::find_widget ()
{
  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      return MW_TRACK_INSPECTOR->sends->slots[slot_];
    }
  return nullptr;
}
#endif

void
ChannelSend::set_port_metadata_from_owner (
  structure::tracks::PortIdentifier &id,
  PortRange                         &range) const
{
  id.set_track_id (track_id_);
  id.port_index_ = slot_;
  id.owner_type_ = structure::tracks::PortIdentifier::OwnerType::ChannelSend;

  if (
    ENUM_BITSET_TEST (
      id.flags2_, structure::tracks::PortIdentifier::Flags2::ChannelSendEnabled))
    {
      range.minf_ = 0.f;
      range.maxf_ = 1.f;
      range.zerof_ = 0.0f;
    }
  else if (
    ENUM_BITSET_TEST (
      id.flags2_, structure::tracks::PortIdentifier::Flags2::ChannelSendAmount))
    {
      range.minf_ = 0.f;
      range.maxf_ = 2.f;
      range.zerof_ = 0.f;
    }
}

ChannelSend *
ChannelSend::find_in_project () const
{
  return get_track ()->channel_->sends_[slot_].get ();
}

void
ChannelSend::append_ports (std::vector<Port *> &ports)
{
  auto add_port = [&] (auto &port_id) {
    if (port_id.has_value ())
      {
        auto port_var = port_id->get_object ();
        std::visit ([&] (auto &&port) { ports.push_back (port); }, port_var);
      }
  };

  add_port (enabled_id_);
  add_port (amount_id_);
  add_port (midi_in_id_);
  add_port (midi_out_id_);
  add_port (stereo_in_left_id_);
  add_port (stereo_in_right_id_);
  add_port (stereo_out_left_id_);
  add_port (stereo_out_right_id_);
}

int
ChannelSend::append_connection (
  const structure::tracks::PortConnectionsManager *             mgr,
  structure::tracks::PortConnectionsManager::ConnectionsVector &arr) const
{
  if (is_empty ())
    return 0;

  if (is_audio ())
    {
      int num_dests =
        mgr->get_sources_or_dests (&arr, stereo_out_left_id_->id (), false);
      z_return_val_if_fail (num_dests == 1, false);
      num_dests =
        mgr->get_sources_or_dests (&arr, stereo_out_right_id_->id (), false);
      z_return_val_if_fail (num_dests == 1, false);
      return 2;
    }
  if (is_midi ())
    {
      int num_dests =
        mgr->get_sources_or_dests (&arr, midi_out_id_->id (), false);
      z_return_val_if_fail (num_dests == 1, false);
      return 1;
    }

  z_return_val_if_reached (0);
}

bool
ChannelSend::is_connected_to (
  std::optional<std::pair<PortUuid, PortUuid>> stereo,
  std::optional<PortUuid>                      midi) const
{
  auto * mgr = get_port_connections_manager ();
  z_return_val_if_fail (mgr, false);

  std::unique_ptr<structure::tracks::PortConnectionsManager::ConnectionsVector>
      conns;
  int num_conns = append_connection (mgr, *conns);
  for (int i = 0; i < num_conns; i++)
    {
      const auto &conn = conns->at (i);
      if (
        (stereo.has_value ()
         && (conn->dest_id_ == stereo->first || conn->dest_id_ == stereo->second))
        || (midi.has_value () && conn->dest_id_ == *midi))
        {
          return true;
        }
    }

  return false;
}
}
