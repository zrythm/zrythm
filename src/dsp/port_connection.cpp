// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_connection.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{

PortConnection::PortConnection (QObject * parent) : QObject (parent) { }

PortConnection::PortConnection (
  const PortUuid &src,
  const PortUuid &dest,
  float           multiplier,
  bool            locked,
  bool            enabled,
  QObject *       parent)
    : QObject (parent), src_id_ (src), dest_id_ (dest),
      multiplier_ (multiplier), locked_ (locked), enabled_ (enabled)
{
  assert (!src.is_null ());
  assert (!dest.is_null ());
}

void
init_from (
  PortConnection        &obj,
  const PortConnection  &other,
  utils::ObjectCloneType clone_type)
{
  obj.src_id_ = other.src_id_;
  obj.dest_id_ = other.dest_id_;
  obj.multiplier_ = other.multiplier_;
  obj.locked_ = other.locked_;
  obj.enabled_ = other.enabled_;
}

void
to_json (nlohmann::json &j, const PortConnection &port_connection)
{
  j[PortConnection::kSourceIdKey] = port_connection.src_id_;
  j[PortConnection::kDestIdKey] = port_connection.dest_id_;
  j[PortConnection::kMultiplierKey] = port_connection.multiplier_;
  j[PortConnection::kLockedKey] = port_connection.locked_;
  j[PortConnection::kEnabledKey] = port_connection.enabled_;
  j[PortConnection::kBipolarKey] = port_connection.bipolar_;
  j[PortConnection::kSourceDestMappingKey] =
    port_connection.source_ch_to_destination_ch_mapping_;
}

void
from_json (const nlohmann::json &j, PortConnection &port_connection)
{
  j.at (PortConnection::kSourceIdKey).get_to (port_connection.src_id_);
  j.at (PortConnection::kDestIdKey).get_to (port_connection.dest_id_);
  j.at (PortConnection::kMultiplierKey).get_to (port_connection.multiplier_);
  j.at (PortConnection::kLockedKey).get_to (port_connection.locked_);
  j.at (PortConnection::kEnabledKey).get_to (port_connection.enabled_);
  j.at (PortConnection::kBipolarKey).get_to (port_connection.bipolar_);
  j.at (PortConnection::kSourceDestMappingKey)
    .get_to (port_connection.source_ch_to_destination_ch_mapping_);
}
}
