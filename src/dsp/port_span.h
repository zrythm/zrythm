// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_all.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::dsp
{
/**
 * @brief Span that offers helper methods on a range of ports.
 */
class PortSpan : public utils::UuidIdentifiableObjectView<dsp::PortRegistry>
{
public:
  using Base = utils::UuidIdentifiableObjectView<dsp::PortRegistry>;
  using VariantType = typename Base::VariantType;
  using PortUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  static auto label_projection (const VariantType &port_var)
  {
    return std::visit (
      [] (const auto &port) { return port->get_label (); }, port_var);
  }
};

static_assert (std::ranges::random_access_range<PortSpan>);
}
