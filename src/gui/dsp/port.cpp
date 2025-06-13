// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/port_connections_manager.h"
#include "dsp/port_identifier.h"
#include "gui/dsp/port_all.h"
#include "utils/hash.h"

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
Port::change_track (IPortOwner::TrackUuid new_track_id)
{
  id_->set_track_id (new_track_id);
}

void
init_from (Port &obj, const Port &other, utils::ObjectCloneType clone_type)
{
  obj.id_ = utils::clone_unique (*other.id_);
  obj.range_ = other.range_;
}

void
Port::print_full_designation () const
{
  z_info ("{}", get_full_designation ());
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
