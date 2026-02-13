// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/port_all.h"

#include <fmt/format.h>

namespace zrythm::dsp
{
Port::Port (utils::Utf8String label, PortType type, PortFlow flow)
    : type_ (type), flow_ (flow), label_ (std::move (label))
{
}

void
init_from (Port &obj, const Port &other, utils::ObjectCloneType clone_type)
{
  // TODO
  // obj.id_ = utils::clone_unique (*other.id_);
}

Port::~Port () = default;
RingBufferOwningPortMixin::~RingBufferOwningPortMixin () = default;

void
to_json (nlohmann::json &j, const Port &p)
{
  to_json (j, static_cast<const Port::UuidIdentifiableObject &> (p));
  j[Port::kTypeId] = p.type_;
  j[Port::kFlowId] = p.flow_;
  j[Port::kLabelId] = p.label_;
  j[Port::kSymbolId] = p.sym_;
  if (p.port_group_)
    {
      j[Port::kPortGroupId] = *p.port_group_;
    }
}

void
from_json (const nlohmann::json &j, Port &p)
{
  from_json (j, static_cast<Port::UuidIdentifiableObject &> (p));
  j.at (Port::kTypeId).get_to (p.type_);
  j.at (Port::kFlowId).get_to (p.flow_);
  j.at (Port::kLabelId).get_to (p.label_);
  j.at (Port::kSymbolId).get_to (p.sym_);
  if (j.contains (Port::kPortGroupId))
    {
      j.at (Port::kPortGroupId).get_to (p.port_group_);
    }
}

} // namespace zrythm::dsp

struct PortRegistryBuilder
{
  template <typename T> std::unique_ptr<T> build () const
  {
    if constexpr (std::is_same_v<T, zrythm::dsp::AudioPort>)
      {
        return std::make_unique<T> (
          u8"", zrythm::dsp::PortFlow{}, zrythm::dsp::AudioPort::BusLayout{}, 0,
          zrythm::dsp::AudioPort::Purpose::Main);
      }
    else
      {
        return std::make_unique<T> (u8"", zrythm::dsp::PortFlow{});
      }
  }
};
void
from_json (const nlohmann::json &j, zrythm::dsp::PortRegistry &registry)
{
  from_json_with_builder (j, registry, PortRegistryBuilder{});
}
