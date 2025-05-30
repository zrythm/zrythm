// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/port.h"
#include "utils/dsp.h"
#include "utils/hash.h"
#include "utils/rt_thread_id.h"

#include <fmt/format.h>

Port::Port () : id_ (std::make_unique<PortIdentifier> ()) { }

Port::Port (
  utils::Utf8String label,
  PortType          type,
  PortFlow          flow,
  float             minf,
  float             maxf,
  float             zerof)
    : Port ()
{
  range_.minf_ = minf;
  range_.maxf_ = maxf;
  range_.zerof_ = zerof;
  id_->label_ = std::move (label);
  id_->type_ = type;
  id_->flow_ = flow;
}

std::unique_ptr<Port>
Port::create_unique_from_type (PortType type)
{
  switch (type)
    {
    case PortType::Audio:
      return std::make_unique<AudioPort> ();
    case PortType::Event:
      return std::make_unique<MidiPort> ();
    case PortType::Control:
      return std::make_unique<ControlPort> ();
    case PortType::CV:
      return std::make_unique<CVPort> ();
    default:
      z_return_val_if_reached (nullptr);
    }
}

bool
Port::is_exposed_to_backend () const
{
  return (is_audio () || is_midi ())
         && ((backend_ && backend_->is_exposed ()) || id_->owner_type_ == PortIdentifier::OwnerType::AudioEngine || exposed_to_backend_);
}

bool
Port::needs_external_buffer_clear_on_early_return () const
{
  return id_->flow_ == dsp::PortFlow::Output && is_exposed_to_backend ();
}

void
Port::process_block (EngineProcessTimeInfo time_nfo)
{
  // FIXME: avoid this mess and calling another process function
  std::visit (
    [&] (auto &&port) {
      using PortT = base_type<decltype (port)>;
      if constexpr (std::is_same_v<PortT, MidiPort>)
        {
          if (ENUM_BITSET_TEST (
                port->id_->flags_, dsp::PortIdentifier::Flags::ManualPress))
            {
              port->midi_events_.dequeue (
                time_nfo.local_offset_, time_nfo.nframes_);
              return;
            }
        }
      if constexpr (std::is_same_v<PortT, AudioPort>)
        {
          if (
            port->id_->is_monitor_fader_stereo_in_or_out_port ()
            && AUDIO_ENGINE->exporting_)
            {
              /* if exporting and the port is not a project port, skip
               * processing */
              return;
            }
        }

      port->process (time_nfo, false);
    },
    convert_to_variant<PortPtrVariant> (this));
}

int
Port::get_num_unlocked (bool sources) const
{
  z_return_val_if_fail (is_in_active_project (), 0);
  return PORT_CONNECTIONS_MGR->get_unlocked_sources_or_dests (
    nullptr, get_uuid (), sources);
}

int
Port::get_num_unlocked_dests () const
{
  return get_num_unlocked (false);
}

int
Port::get_num_unlocked_srcs () const
{
  return get_num_unlocked (true);
}

void
Port::set_owner (IPortOwner &owner)
{
  owner_ = std::addressof (owner);
  owner_->set_port_metadata_from_owner (*id_, range_);
}

utils::Utf8String
Port::get_label () const
{
  return id_->get_label ();
}

void
Port::disconnect_all ()
{
  srcs_.clear ();
  dests_.clear ();

  if (!gZrythm || !PROJECT || !PORT_CONNECTIONS_MGR)
    return;

  if (!is_in_active_project ())
    {
#if 0
      z_debug ("{} ({}) is not a project port, skipping", this->id_.label, fmt::ptr(this));
#endif
      return;
    }

  dsp::PortConnectionsManager::ConnectionsVector srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, get_uuid (), true);
  for (const auto &conn : srcs)
    {
      PORT_CONNECTIONS_MGR->remove_connection (conn->src_id_, conn->dest_id_);
    }

  dsp::PortConnectionsManager::ConnectionsVector dests;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&dests, get_uuid (), false);
  for (const auto &conn : dests)
    {
      PORT_CONNECTIONS_MGR->remove_connection (conn->src_id_, conn->dest_id_);
    }

  if (backend_)
    {
      backend_->unexpose ();
    }
}

void
Port::change_track (IPortOwner::TrackUuid new_track_id)
{
  id_->set_track_id (new_track_id);
}

void
Port::copy_members_from (const Port &other, ObjectCloneType clone_type)
{
  id_ = other.id_->clone_unique ();
  exposed_to_backend_ = other.exposed_to_backend_;
  range_ = other.range_;
}

void
Port::print_full_designation () const
{
  z_info ("{}", get_full_designation ());
}

void
Port::clear_external_buffer ()
{
  if (!is_exposed_to_backend () || !backend_)
    {
      return;
    }

  backend_->clear_backend_buffer (id_->type_, AUDIO_ENGINE->block_length_);
}

size_t
Port::get_hash () const
{
  return utils::hash::get_object_hash (*this);
}

struct PortRegistryBuilder
{
  template <typename T> std::unique_ptr<T> build () const
  {
    return std::make_unique<T> ();
  }
};

void
from_json (const nlohmann::json &j, PortRegistry &registry)
{
  from_json_with_builder (j, registry, PortRegistryBuilder{});
}
