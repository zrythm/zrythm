// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/port_all.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
Port::
  Port (utils::Utf8String label, PortType type, PortFlow flow, QObject * parent)
    : utils::UuidIdentifiableObject<Port> (parent), type_ (type), flow_ (flow),
      label_ (std::move (label))
{
}

void
init_from (Port &obj, const Port &other, utils::ObjectCloneType clone_type)
{
  // TODO
  // obj.id_ = utils::clone_unique (*other.id_);
}

Port::~Port () = default;

void
Port::set_label (utils::Utf8String new_label)
{
  if (label_ == new_label)
    return;
  label_ = std::move (new_label);
  Q_EMIT labelChanged (label ());
}

void
Port::set_detached (bool new_detached)
{
  if (detached_ == new_detached)
    return;
  detached_ = new_detached;
  Q_EMIT detachedChanged (detached_);
}

void
to_json (nlohmann::json &j, const Port &p)
{
  to_json (j, static_cast<const Port::UuidIdentifiableObject &> (p));
  j[Port::kFlowId] = p.flow_;
  j[Port::kLabelId] = p.label_;
  j[Port::kSymbolId] = p.sym_;
  if (p.detached_)
    {
      j[Port::kDetachedId] = true;
    }
  if (p.port_group_)
    {
      j[Port::kPortGroupId] = *p.port_group_;
    }
}

void
from_json (const nlohmann::json &j, Port &p)
{
  from_json (j, static_cast<Port::UuidIdentifiableObject &> (p));
  j.at (Port::kFlowId).get_to (p.flow_);
  j.at (Port::kLabelId).get_to (p.label_);
  j.at (Port::kSymbolId).get_to (p.sym_);
  if (j.contains (Port::kDetachedId))
    {
      j.at (Port::kDetachedId).get_to (p.detached_);
    }
  if (j.contains (Port::kPortGroupId))
    {
      j.at (Port::kPortGroupId).get_to (p.port_group_);
    }
}

} // namespace zrythm::dsp
