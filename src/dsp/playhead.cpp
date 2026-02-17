// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/playhead.h"

namespace zrythm::dsp
{
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
