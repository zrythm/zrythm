// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/playhead.h"

#include <au/math.hh>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
void
Playhead::set_position_ticks (units::precise_tick_t ticks)
{
  std::lock_guard lock (position_mutex_);
  position_ticks_ = max (ticks, units::ticks (0.0));
  position_samples_.store (
    tempo_map_.tick_to_samples (TimelineTick{ position_ticks_ }),
    std::memory_order_release);
}

void
to_json (nlohmann::json &j, const Playhead &pos)
{
  j[Playhead::kMode] = 0;
  j[Playhead::kValue] = pos.position_ticks ().in (units::ticks);
}
void
from_json (const nlohmann::json &j, Playhead &pos)
{
  double ticks{};
  j.at (Playhead::kValue).get_to (ticks);
  pos.set_position_ticks (units::ticks (ticks));
}
}
