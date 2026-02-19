// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
void
to_json (nlohmann::json &j, const AtomicPosition &pos)
{
  const auto &[d, b] = pos.value_.load ();
  const auto mode = AtomicPosition::bool_to_format (b);
  // Only serialize mode if not default (Musical)
  if (mode != TimeFormat::Musical)
    {
      j[AtomicPosition::kMode] = mode;
    }
  j[AtomicPosition::kValue] = d;
}
void
from_json (const nlohmann::json &j, AtomicPosition &pos)
{
  double value{};
  j.at (AtomicPosition::kValue).get_to (value);
  // Default to Musical if mode not present
  TimeFormat mode = TimeFormat::Musical;
  if (j.contains (AtomicPosition::kMode))
    {
      j.at (AtomicPosition::kMode).get_to (mode);
    }
  pos.value_.store (value, AtomicPosition::format_to_bool (mode));
}

} // namespace zrythm::dsp
