// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/port_all.h"
#include "utils/uuid_identifiable_object.h"

/**
 * @brief Track span that offers helper methods on a range of tracks.
 */
class PortSpan : public utils::UuidIdentifiableObjectView<PortRegistry>
{
public:
  using Base = utils::UuidIdentifiableObjectView<PortRegistry>;
  using VariantType = typename Base::VariantType;
  using PortUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

#if 0
  static auto name_projection (const VariantType &port_var)
  {
    return std::visit (
      [] (const auto &port) { return port->get_name (); }, port_var);
  }
#endif
  static auto identifier_projection (const VariantType &port_var)
  {
    return std::visit (
      [] (const auto &port) { return port->id_.get (); }, port_var);
  }
};

static_assert (std::ranges::random_access_range<PortSpan>);
