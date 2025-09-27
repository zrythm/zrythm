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

} // namespace zrythm::dsp

struct PortRegistryBuilder
{
  template <typename T> std::unique_ptr<T> build () const
  {
    if constexpr (std::is_same_v<T, zrythm::dsp::AudioPort>)
      {
        return std::make_unique<T> (
          u8"", zrythm::dsp::PortFlow::Unknown,
          zrythm::dsp::AudioPort::BusLayout::Unknown, 0,
          zrythm::dsp::AudioPort::Purpose::Main);
      }
    else
      {
        return std::make_unique<T> (u8"", zrythm::dsp::PortFlow::Unknown);
      }
  }
};
void
from_json (const nlohmann::json &j, zrythm::dsp::PortRegistry &registry)
{
  from_json_with_builder (j, registry, PortRegistryBuilder{});
}
