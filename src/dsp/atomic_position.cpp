// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
void
to_json (nlohmann::json &j, const AtomicPosition &pos)
{
  const auto &[d, b] = pos.value_.load ();
  j[AtomicPosition::kMode] = AtomicPosition::bool_to_format (b);
  j[AtomicPosition::kValue] = d;
}
void
from_json (const nlohmann::json &j, AtomicPosition &pos)
{
  TimeFormat mode{};
  double     value{};
  j.at (AtomicPosition::kMode).get_to (mode);
  j.at (AtomicPosition::kValue).get_to (value);
  pos.value_.store (value, AtomicPosition::format_to_bool (mode));
}

} // namespace zrythm::dsp
